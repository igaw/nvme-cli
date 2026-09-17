// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * privsep helper (issue #3879): the process that will eventually run
 * privileged and issue the real ioctl. Through Phase 2 it was invoked
 * manually by the test harness (libnvme/tests/privsep/parent.c) over a
 * socket inherited at a fixed fd; as of Phase 3, nvme-cli's own
 * src/privsep-lifecycle.c spawns it the same way, at startup, before any
 * device name is known -- nothing in this phase installs it anywhere yet.
 * No allowlist, O_NOFOLLOW, or capability/seccomp hardening yet: that's
 * Phase 4. It reuses the existing, already-shipped
 * libnvme_open()/libnvme_exec_*_passthru() path verbatim -- no path
 * validation is reinvented here.
 *
 * Session model (Phase 3): the helper starts with no device open. A
 * LIBNVME_PRIVSEP_OP_OPEN_DEVICE request (see handle_open_device() below)
 * opens one, closing whatever was previously open first -- one "current"
 * device per session, not a table. ADMIN/IO requests before the first
 * successful OPEN_DEVICE get -ENODEV.
 *
 * Test-only self-configuration: when told to open the literal sentinel
 * device name "NVME_TEST_FD"/"NVME_TEST_FD64" -- a mechanism libnvme_open()
 * itself already special-cases unconditionally (lib-linux.c) -- this
 * helper additionally enables dry_run and installs a small deterministic
 * submit_exit responder, so a round trip can be checked without a real
 * device. This is new: nothing else in the repo exercises NVME_TEST_FD or
 * dry_run. It validates marshal/unmarshal, PRIVSEP dispatch, the real
 * fork/exec/socket lifecycle, and the 32-vs-64-bit ioctl state machine's
 * *code path selection* -- not real kernel ioctl() behavior, since
 * dry_run skips the syscall entirely.
 *
 * Phase 2 addition: LIBNVME_PRIVSEP_OP_FABRICS_CONNECT relays a fabrics
 * connect by calling the real, unmodified __nvmf_add_ctrl() (fabrics.c) --
 * see handle_fabrics_connect() below. There is no dry-run double for it;
 * on a machine without /dev/nvme-fabrics this simply returns a clean
 * negative errno, which is enough to validate the relay itself.
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libnvme.h>

#include "nvme/privsep-proto.h"
#ifdef CONFIG_FABRICS
#include "nvme/private-fabrics.h"
#endif

#define PRIVSEP_SOCK_FD 3

static void test_submit_exit(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd, int err, void *user_data)
{
	uint8_t *data = (uint8_t *)(uintptr_t)cmd->addr;
	uint32_t i;

	cmd->result = 0xA5A5A5A5u ^ cmd->opcode;
	for (i = 0; i < cmd->data_len; i++)
		data[i] = (uint8_t)(i ^ cmd->opcode);
}

#ifdef CONFIG_FABRICS
static void handle_fabrics_connect(struct libnvme_global_ctx *ctx,
		const struct libnvme_privsep_req *req,
		struct libnvme_privsep_resp *resp)
{
	/* req->data is the already-built option string (build_options(), in
	 * fabrics.c) crossing the boundary as-is -- this never (re)builds it.
	 */
	int ret = __nvmf_add_ctrl(ctx, (const char *)req->data);

	if (ret >= 0) {
		resp->status = 0;
		resp->result = (uint64_t)ret;
	} else {
		resp->status = ret;
	}
}
#else
static void handle_fabrics_connect(struct libnvme_global_ctx *ctx,
		const struct libnvme_privsep_req *req,
		struct libnvme_privsep_resp *resp)
{
	resp->status = -ENOTSUP;
}
#endif

/*
 * req->data is the NUL-terminated device name (guaranteed by the sender,
 * __libnvme_privsep_open_device()); req->cdw10 is the open() flags.
 */
static void handle_open_device(struct libnvme_global_ctx *ctx,
		struct libnvme_transport_handle **hdlp,
		const struct libnvme_privsep_req *req,
		struct libnvme_privsep_resp *resp)
{
	const char *devname = (const char *)req->data;
	bool is_test_fd = !strncmp(devname, "NVME_TEST_FD", 12);
	struct libnvme_transport_handle *hdl;
	int ret;

	if (*hdlp) {
		libnvme_close(*hdlp);
		*hdlp = NULL;
	}

	/* Always set, not just when true: a later real open in the same
	 * session must not inherit dry_run left on by an earlier test open.
	 */
	libnvme_set_dry_run(ctx, is_test_fd);

	ret = libnvme_open(ctx, devname, (int)req->cdw10, &hdl);
	if (ret) {
		resp->status = ret;
		return;
	}

	if (is_test_fd)
		libnvme_transport_handle_set_submit_exit(hdl, test_submit_exit);

	*hdlp = hdl;
	resp->status = 0;
}

static void handle_one(struct libnvme_global_ctx *ctx,
		struct libnvme_transport_handle *hdl,
		const struct libnvme_privsep_req *req,
		struct libnvme_privsep_resp *resp)
{
	struct libnvme_passthru_cmd cmd = {
		.opcode = req->opcode,
		.flags = req->flags,
		.nsid = req->nsid,
		.cdw2 = req->cdw2,
		.cdw3 = req->cdw3,
		.cdw10 = req->cdw10,
		.cdw11 = req->cdw11,
		.cdw12 = req->cdw12,
		.cdw13 = req->cdw13,
		.cdw14 = req->cdw14,
		.cdw15 = req->cdw15,
		.timeout_ms = req->timeout_ms,
		.data_len = req->data_len,
		.addr = (uint64_t)(uintptr_t)resp->data,
	};

	if (req->data_len)
		memcpy(resp->data, req->data, req->data_len);

	switch (req->op) {
	case LIBNVME_PRIVSEP_OP_ADMIN:
		resp->status = libnvme_exec_admin_passthru(hdl, &cmd);
		break;
	case LIBNVME_PRIVSEP_OP_IO:
		resp->status = libnvme_exec_io_passthru(hdl, &cmd);
		break;
	default:
		resp->status = -EINVAL;
		return;
	}

	resp->result = cmd.result;
	resp->data_len = req->data_len;
}

/* Dispatches every request type, including the two (OPEN_DEVICE,
 * FABRICS_CONNECT) that don't operate on an already-open passthru handle.
 */
static void dispatch(struct libnvme_global_ctx *ctx,
		struct libnvme_transport_handle **hdlp,
		const struct libnvme_privsep_req *req,
		struct libnvme_privsep_resp *resp)
{
	switch (req->op) {
	case LIBNVME_PRIVSEP_OP_OPEN_DEVICE:
		handle_open_device(ctx, hdlp, req, resp);
		break;
	case LIBNVME_PRIVSEP_OP_FABRICS_CONNECT:
		handle_fabrics_connect(ctx, req, resp);
		break;
	default:
		handle_one(ctx, *hdlp, req, resp);
		break;
	}
}

int main(int argc, char **argv)
{
	struct libnvme_global_ctx *ctx;
	struct libnvme_transport_handle *hdl = NULL;
	struct libnvme_privsep_req *req = malloc(sizeof(*req));
	struct libnvme_privsep_resp *resp = malloc(sizeof(*resp));
	int ret;

	if (!req || !resp) {
		fprintf(stderr, "%s: out of memory\n", argv[0]);
		return 2;
	}

	ctx = libnvme_create_global_ctx();

	/* Test-only: force the 32-bit ioctl state machine for the whole
	 * session (libnvme_set_ioctl_probing()'s doc explains why probing
	 * alone can't reach it under dry_run). Real invocations never pass
	 * this; nvme-cli's own spawner (src/privsep-lifecycle.c) doesn't.
	 */
	if (argc > 1 && !strcmp(argv[1], "32"))
		libnvme_set_ioctl_probing(ctx, false);

	for (;;) {
		memset(resp, 0, offsetof(struct libnvme_privsep_resp, data));

		ret = libnvme_privsep_recv_req(PRIVSEP_SOCK_FD, req);
		if (ret == 0)
			break; /* parent closed the socket: normal shutdown */

		if (ret == -EMSGSIZE) {
			resp->status = -EMSGSIZE;
			libnvme_privsep_send_resp(PRIVSEP_SOCK_FD, resp);
			continue;
		}
		if (ret < 0)
			break; /* garbled message: not a well-formed peer, stop */

		dispatch(ctx, &hdl, req, resp);
		libnvme_privsep_send_resp(PRIVSEP_SOCK_FD, resp);
	}

	if (hdl)
		libnvme_close(hdl);
	libnvme_free_global_ctx(ctx);
	free(req);
	free(resp);

	return 0;
}
