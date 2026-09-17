/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libnvme.
 *
 * Wire protocol for LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP (issue #3879,
 * Phase 1): a dedicated request/response struct crossing a connected
 * SOCK_SEQPACKET socket -- never a repurposed struct libnvme_passthru_cmd
 * field. Promoted from the Phase 0 spike's validated shape
 * (libnvme/tests/privsep-spike/proto.h), now carrying real admin/IO
 * passthru commands instead of a mock opcode.
 *
 * Every message on the wire is its fixed header plus exactly data_len
 * bytes -- never the full LIBNVME_PRIVSEP_MAX_XFER capacity -- so cost
 * scales with the real payload size, not the compile-time cap.
 *
 * No data-direction flag: the request always carries the caller's current
 * buffer content, and the response always carries the buffer's content
 * after the real exec call. This matches how a real ioctl already treats
 * cmd->addr (the kernel doesn't need a direction flag either -- it's
 * implied by the opcode), and avoids this marshal layer having to
 * classify every admin/IO opcode's direction itself.
 *
 * Not yet handled (Phase 1 scope: direct-attached admin/IO passthru only):
 * cmd->metadata / metadata_len. No admin or IO command nvme-cli issues via
 * this path today needs a separate metadata buffer; add one alongside
 * data if/when that changes, rather than guessing at the shape now.
 */
#pragma once

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define LIBNVME_PRIVSEP_MAX_XFER (4 * 1024 * 1024)

enum libnvme_privsep_op {
	LIBNVME_PRIVSEP_OP_ADMIN = 1,
	LIBNVME_PRIVSEP_OP_IO,
	/*
	 * Fabrics connect (issue #3879 Phase 2): req->data carries the
	 * already-built, NUL-terminated option string (see build_options()
	 * in fabrics.c, which runs only on the unprivileged side -- never
	 * here), req->data_len = strlen(argstr) + 1. resp->result carries
	 * the parsed controller instance number on success; resp->status
	 * carries 0 or a negative errno exactly like the passthru ops.
	 */
	LIBNVME_PRIVSEP_OP_FABRICS_CONNECT,
};

struct libnvme_privsep_req {
	uint32_t op;		/* enum libnvme_privsep_op */
	uint8_t  opcode;
	uint8_t  flags;
	uint32_t nsid;
	uint32_t cdw2;
	uint32_t cdw3;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
	uint32_t timeout_ms;
	uint32_t data_len;	/* must be <= LIBNVME_PRIVSEP_MAX_XFER */
	uint8_t  data[LIBNVME_PRIVSEP_MAX_XFER];
};

struct libnvme_privsep_resp {
	int32_t  status;	/* return value of the real exec call */
	uint64_t result;	/* struct libnvme_passthru_cmd.result */
	uint32_t data_len;
	uint8_t  data[LIBNVME_PRIVSEP_MAX_XFER];
};

static inline size_t libnvme_privsep_req_len(const struct libnvme_privsep_req *req)
{
	uint32_t len = req->data_len;

	if (len > LIBNVME_PRIVSEP_MAX_XFER)
		len = LIBNVME_PRIVSEP_MAX_XFER;

	return offsetof(struct libnvme_privsep_req, data) + len;
}

static inline size_t libnvme_privsep_resp_len(const struct libnvme_privsep_resp *resp)
{
	uint32_t len = resp->data_len;

	if (len > LIBNVME_PRIVSEP_MAX_XFER)
		len = LIBNVME_PRIVSEP_MAX_XFER;

	return offsetof(struct libnvme_privsep_resp, data) + len;
}

static inline ssize_t libnvme_privsep_send_req(int sock,
		const struct libnvme_privsep_req *req)
{
	return send(sock, req, libnvme_privsep_req_len(req), 0);
}

static inline ssize_t libnvme_privsep_send_resp(int sock,
		const struct libnvme_privsep_resp *resp)
{
	return send(sock, resp, libnvme_privsep_resp_len(resp), 0);
}

/*
 * Receives one message into @buf (capacity @bufsize) and validates it
 * before the caller ever looks at data_len:
 *
 *   0         -- the peer closed the socket (clean shutdown)
 *   1         -- a well-formed message was received
 *   -EIO      -- garbled: too short, or the header's data_len doesn't
 *                match the number of bytes actually received
 *   -EMSGSIZE -- structurally fine, but data_len exceeds
 *                LIBNVME_PRIVSEP_MAX_XFER; the caller should reply with
 *                that status rather than just drop the connection
 */
static inline int libnvme_privsep_recv(int sock, void *buf, size_t bufsize,
		size_t hdr_len, size_t data_len_off)
{
	uint32_t data_len;
	ssize_t n = recv(sock, buf, bufsize, 0);

	if (n == 0)
		return 0;
	if (n < 0 || (size_t)n < hdr_len)
		return -EIO;

	memcpy(&data_len, (const uint8_t *)buf + data_len_off, sizeof(data_len));

	if (data_len > LIBNVME_PRIVSEP_MAX_XFER)
		return -EMSGSIZE;
	if ((size_t)n != hdr_len + data_len)
		return -EIO;

	return 1;
}

#define libnvme_privsep_recv_req(sock, req) \
	libnvme_privsep_recv((sock), (req), sizeof(*(req)), \
			     offsetof(struct libnvme_privsep_req, data), \
			     offsetof(struct libnvme_privsep_req, data_len))

#define libnvme_privsep_recv_resp(sock, resp) \
	libnvme_privsep_recv((sock), (resp), sizeof(*(resp)), \
			     offsetof(struct libnvme_privsep_resp, data), \
			     offsetof(struct libnvme_privsep_resp, data_len))

/*
 * AF_UNIX SOCK_SEQPACKET's default SO_SNDBUF/SO_RCVBUF (~208KB on Linux) is
 * well under LIBNVME_PRIVSEP_MAX_XFER: a single send() of a larger message
 * fails with EMSGSIZE at the kernel level before either side's code ever
 * runs (found empirically in Phase 0). setsockopt() silently clamps to
 * net.core.wmem_max/rmem_max, so on a system where those are below
 * LIBNVME_PRIVSEP_MAX_XFER, the largest single message this protocol can
 * actually carry is smaller than the compile-time cap.
 */
static inline void libnvme_privsep_size_socket_buffers(int sv[2])
{
	int i;

	for (i = 0; i < 2; i++) {
		setsockopt(sv[i], SOL_SOCKET, SO_SNDBUF,
			   &(int){ LIBNVME_PRIVSEP_MAX_XFER }, sizeof(int));
		setsockopt(sv[i], SOL_SOCKET, SO_RCVBUF,
			   &(int){ LIBNVME_PRIVSEP_MAX_XFER }, sizeof(int));
	}
}
