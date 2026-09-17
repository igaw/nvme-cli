// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Phase 1 privsep helper (issue #3879): the process that will eventually
 * run privileged and issue the real ioctl. For this phase it is invoked
 * manually by the test harness (libnvme/tests/privsep/parent.c) over a
 * socket inherited at a fixed fd -- nothing in this phase spawns it
 * automatically or installs it. No allowlist, O_NOFOLLOW, or
 * capability/seccomp hardening yet: that's Phase 4. It reuses the
 * existing, already-shipped libnvme_open()/libnvme_exec_*_passthru() path
 * verbatim -- no path validation is reinvented here.
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

static void handle_one(struct libnvme_transport_handle *hdl,
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

int main(int argc, char **argv)
{
	struct libnvme_global_ctx *ctx;
	struct libnvme_transport_handle *hdl;
	struct libnvme_privsep_req *req = malloc(sizeof(*req));
	struct libnvme_privsep_resp *resp = malloc(sizeof(*resp));
	const char *devname;
	bool is_test_fd;
	int ret;

	if (argc < 2 || !req || !resp) {
		fprintf(stderr, "usage: %s <devname> [32]\n", argv[0]);
		return 2;
	}
	devname = argv[1];
	is_test_fd = !strncmp(devname, "NVME_TEST_FD", 12);

	ctx = libnvme_create_global_ctx();

	if (is_test_fd) {
		libnvme_set_dry_run(ctx, true);
		if (argc > 2 && !strcmp(argv[2], "32"))
			libnvme_set_ioctl_probing(ctx, false);
	}

	ret = libnvme_open(ctx, devname, 0, &hdl);
	if (ret) {
		fprintf(stderr, "%s: libnvme_open(%s) failed: %d\n",
			argv[0], devname, ret);
		return 1;
	}

	if (is_test_fd)
		libnvme_transport_handle_set_submit_exit(hdl, test_submit_exit);

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

		handle_one(hdl, req, resp);
		libnvme_privsep_send_resp(PRIVSEP_SOCK_FD, resp);
	}

	libnvme_close(hdl);
	libnvme_free_global_ctx(ctx);
	free(req);
	free(resp);

	return 0;
}
