/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Throwaway Phase-0 spike for the privilege-separation design in GitHub
 * issue #3879 (nvme-cli privsep). Not shipped: built only under
 * -Dprivsep-spike=true, never linked into any default target.
 *
 * Wire protocol: a dedicated request/response struct, deliberately not a
 * repurposed struct libnvme_passthru_cmd. Every message the receiver reads
 * is one complete, immutable copy already sitting in its own memory by the
 * time recv() returns -- there is no shared/writable mapping between the
 * two sides at any point.
 */
#pragma once

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

struct libnvme_transport_handle;

/*
 * Deliberately generous: lets the bench harness sweep up to telemetry-scale
 * transfers. The correctness tests only ever use a few KB of it.
 */
#define SPIKE_MAX_XFER (4 * 1024 * 1024)

/*
 * A vendor/custom admin opcode with no real NVMe meaning. Used only to
 * exercise this spike's write-direction (input-copy) path, since neither
 * Identify nor Get Log Page needs one.
 */
#define SPIKE_MOCK_WRITE_OPCODE 0xC1

struct privsep_req {
	uint8_t  opcode;
	uint32_t nsid;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
	uint32_t data_len;	/* must be <= SPIKE_MAX_XFER */
	uint8_t  data[SPIKE_MAX_XFER];
};

struct privsep_resp {
	int32_t  status;	/* 0, or -errno/NVMe status from the exec call */
	uint32_t result;	/* CQE DW0 */
	uint32_t data_len;	/* bytes valid in data[] */
	uint8_t  data[SPIKE_MAX_XFER];
};

/* The privileged side of the spike: services requests off @sock against
 * @hdl (a loopback transport handle in this spike) until the socket closes.
 */
void child_main(int sock, struct libnvme_transport_handle *hdl);

/*
 * Framing helpers shared by both sides. Each message on the wire is only
 * the fixed header plus exactly data_len bytes -- never the full
 * SPIKE_MAX_XFER capacity -- so round-trip cost genuinely scales with the
 * logical payload size instead of always moving the full buffer.
 *
 * The *_len() helpers clamp to SPIKE_MAX_XFER: a caller building a
 * deliberately out-of-bounds data_len (to test rejection) must not cause
 * the sender itself to read past the end of its own buffer. The receiver
 * is responsible for rejecting an out-of-bounds data_len on its own terms;
 * clamping here only protects the local send() call.
 */

static inline size_t privsep_req_len(const struct privsep_req *req)
{
	uint32_t len = req->data_len;

	if (len > SPIKE_MAX_XFER)
		len = SPIKE_MAX_XFER;

	return offsetof(struct privsep_req, data) + len;
}

static inline size_t privsep_resp_len(const struct privsep_resp *resp)
{
	uint32_t len = resp->data_len;

	if (len > SPIKE_MAX_XFER)
		len = SPIKE_MAX_XFER;

	return offsetof(struct privsep_resp, data) + len;
}

static inline ssize_t privsep_send_req(int sock, const struct privsep_req *req)
{
	return send(sock, req, privsep_req_len(req), 0);
}

static inline ssize_t privsep_send_resp(int sock, const struct privsep_resp *resp)
{
	return send(sock, resp, privsep_resp_len(resp), 0);
}

/*
 * Receives one message into @buf (capacity sizeof(*buf)) and validates it
 * before the caller ever looks at data_len:
 *
 *   0   -- the peer closed the socket (clean shutdown)
 *   1   -- a well-formed message was received
 *   -EIO      -- garbled: too short, or the header's data_len doesn't
 *                match the number of bytes actually received
 *   -EMSGSIZE -- structurally fine, but data_len exceeds SPIKE_MAX_XFER;
 *                the caller should reply with that status, not just drop
 *                the connection, since the peer may be well-behaved code
 *                probing the boundary rather than an attacker
 *
 * @hdr_len is offsetof(..., data) for whichever of the two struct types
 * @buf points to, and @data_len_off is the byte offset of that struct's
 * data_len field -- passed in rather than templated, since C has no
 * generics.
 */
static inline int privsep_recv(int sock, void *buf, size_t bufsize,
				size_t hdr_len, size_t data_len_off)
{
	uint32_t data_len;
	ssize_t n = recv(sock, buf, bufsize, 0);

	if (n == 0)
		return 0;
	if (n < 0 || (size_t)n < hdr_len)
		return -EIO;

	memcpy(&data_len, (const uint8_t *)buf + data_len_off, sizeof(data_len));

	if (data_len > SPIKE_MAX_XFER)
		return -EMSGSIZE;
	if ((size_t)n != hdr_len + data_len)
		return -EIO;

	return 1;
}

#define privsep_recv_req(sock, req) \
	privsep_recv((sock), (req), sizeof(*(req)), \
		     offsetof(struct privsep_req, data), \
		     offsetof(struct privsep_req, data_len))

#define privsep_recv_resp(sock, resp) \
	privsep_recv((sock), (resp), sizeof(*(resp)), \
		     offsetof(struct privsep_resp, data), \
		     offsetof(struct privsep_resp, data_len))

/*
 * AF_UNIX SOCK_SEQPACKET's default SO_SNDBUF/SO_RCVBUF (~208KB on Linux) is
 * well under SPIKE_MAX_XFER: a single send() of a larger message fails
 * with EMSGSIZE at the kernel level before either side's code ever runs.
 * This is a real constraint the real design will hit too -- discovered by
 * this spike, not assumed -- so both ends of the pair need their buffers
 * sized up front to carry the largest message this protocol allows.
 * setsockopt() silently clamps to net.core.wmem_max/rmem_max, so on a
 * system where those are below SPIKE_MAX_XFER, the largest single message
 * this protocol can actually carry is smaller than the compile-time cap;
 * Phase 1 needs to pick its real cap with that in mind, or chunk.
 */
static inline void privsep_size_socket_buffers(int sv[2])
{
	int i;

	for (i = 0; i < 2; i++) {
		setsockopt(sv[i], SOL_SOCKET, SO_SNDBUF,
			   &(int){ SPIKE_MAX_XFER }, sizeof(int));
		setsockopt(sv[i], SOL_SOCKET, SO_RCVBUF,
			   &(int){ SPIKE_MAX_XFER }, sizeof(int));
	}
}
