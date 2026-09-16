// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Phase-0 privsep spike (issue #3879): the "unprivileged" side.
 *
 * Forks a child (child.c) connected over a SOCK_SEQPACKET socketpair, and
 * drives it through Identify Controller, Get Log Page, and a synthetic
 * write-direction mock command -- repeatedly, over the same persistent
 * child, the way one long-running nvme-cli invocation issuing several
 * admin commands would. Also checks that an oversized request is rejected
 * cleanly. All against the existing in-process loopback transport: no root,
 * no real device, no shipped code path touched.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libnvme.h>

#include "nvme/loopback.h"
#include "proto.h"
#include "util.h"

#define ROUNDS 5

static struct nvme_id_ctrl expected_id;
static struct nvme_smart_log expected_smart;
static uint8_t expected_write_in[256];

static void install_mock_sequence(struct libnvme_transport_handle *hdl)
{
	static struct libnvme_loopback_cmd cmds[3 * ROUNDS];
	int i;

	arbitrary(&expected_id, sizeof(expected_id));
	arbitrary(&expected_smart, sizeof(expected_smart));
	arbitrary(expected_write_in, sizeof(expected_write_in));

	for (i = 0; i < ROUNDS; i++) {
		cmds[3 * i] = (struct libnvme_loopback_cmd){
			.opcode = nvme_admin_identify,
			.cdw10 = NVME_IDENTIFY_CNS_CTRL,
			.data_len = sizeof(expected_id),
			.out_data = &expected_id,
		};
		cmds[3 * i + 1] = (struct libnvme_loopback_cmd){
			.opcode = nvme_admin_get_log_page,
			.nsid = NVME_NSID_ALL,
			.cdw10 = (uint32_t)(NVME_LOG_LID_SMART |
				(((sizeof(expected_smart) >> 2) - 1) << 16)),
			.data_len = sizeof(expected_smart),
			.out_data = &expected_smart,
		};
		cmds[3 * i + 2] = (struct libnvme_loopback_cmd){
			.opcode = SPIKE_MOCK_WRITE_OPCODE,
			.data_len = sizeof(expected_write_in),
			.in_data = expected_write_in,
		};
	}

	libnvme_loopback_set_admin_cmds(hdl, cmds, 3 * ROUNDS);
}

static void run_round(int sock, struct privsep_req *req, struct privsep_resp *resp,
		       const char *what, const void *expect_data, size_t expect_len)
{
	int ret;

	check(privsep_send_req(sock, req) == (ssize_t)privsep_req_len(req),
	      "%s: send failed: %s", what, strerror(errno));

	ret = privsep_recv_resp(sock, resp);
	check(ret == 1, "%s: recv failed or rejected (%d)", what, ret);
	check(resp->status == 0, "%s: child reported status %d", what, resp->status);

	if (expect_data)
		cmp(resp->data, expect_data, expect_len,
		    "unexpected round-trip data");
}

static void run_identify(int sock, struct privsep_req *req, struct privsep_resp *resp)
{
	memset(req, 0, offsetof(struct privsep_req, data));
	req->opcode = nvme_admin_identify;
	req->cdw10 = NVME_IDENTIFY_CNS_CTRL;
	req->data_len = sizeof(expected_id);

	run_round(sock, req, resp, "identify", &expected_id, sizeof(expected_id));
}

static void run_get_log_smart(int sock, struct privsep_req *req, struct privsep_resp *resp)
{
	memset(req, 0, offsetof(struct privsep_req, data));
	req->opcode = nvme_admin_get_log_page;
	req->nsid = NVME_NSID_ALL;
	req->cdw10 = (uint32_t)(NVME_LOG_LID_SMART |
		(((sizeof(expected_smart) >> 2) - 1) << 16));
	req->data_len = sizeof(expected_smart);

	run_round(sock, req, resp, "get-log-smart", &expected_smart, sizeof(expected_smart));
}

static void run_write_mock(int sock, struct privsep_req *req, struct privsep_resp *resp)
{
	memset(req, 0, offsetof(struct privsep_req, data));
	req->opcode = SPIKE_MOCK_WRITE_OPCODE;
	req->data_len = sizeof(expected_write_in);
	memcpy(req->data, expected_write_in, sizeof(expected_write_in));

	run_round(sock, req, resp, "write-mock", NULL, 0);
}

static void run_oversized_rejection(int sock, struct privsep_req *req,
				     struct privsep_resp *resp)
{
	ssize_t n;
	int ret;

	memset(req, 0, offsetof(struct privsep_req, data));
	req->opcode = nvme_admin_get_log_page;
	req->nsid = NVME_NSID_ALL;
	req->data_len = SPIKE_MAX_XFER + 1; /* deliberately invalid */

	/*
	 * Only the header needs to cross the wire: the rejection is decided
	 * from the data_len field's value alone, so this test doesn't need
	 * to (and to stay independent of any particular socket buffer
	 * sizing, shouldn't) actually transmit SPIKE_MAX_XFER bytes.
	 */
	n = send(sock, req, offsetof(struct privsep_req, data), 0);
	check(n > 0, "oversized: send failed: %s", strerror(errno));

	ret = privsep_recv_resp(sock, resp);
	check(ret == 1, "oversized: recv failed (%d)", ret);
	check(resp->status == -EMSGSIZE,
	      "oversized request wasn't rejected with -EMSGSIZE (got %d)",
	      resp->status);
}

int main(void)
{
	struct libnvme_global_ctx *ctx;
	struct libnvme_transport_handle *hdl;
	struct privsep_req *req = malloc(sizeof(*req));
	struct privsep_resp *resp = malloc(sizeof(*resp));
	int sv[2];
	pid_t child_pid;
	int i, status;

	check(req && resp, "out of memory");

	ctx = libnvme_create_global_ctx();
	libnvme_set_logging_file(ctx, stdout);
	check(!libnvme_open_loopback(ctx, &hdl), "opening loopback failed");
	install_mock_sequence(hdl);

	check(!socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv),
	      "socketpair failed: %s", strerror(errno));
	privsep_size_socket_buffers(sv);

	child_pid = fork();
	check(child_pid >= 0, "fork failed: %s", strerror(errno));

	if (child_pid == 0) {
		close(sv[0]);
		child_main(sv[1], hdl);
		_exit(0);
	}

	close(sv[1]);

	for (i = 0; i < ROUNDS; i++) {
		printf("round %d/%d...", i + 1, ROUNDS);
		fflush(stdout);
		run_identify(sv[0], req, resp);
		run_get_log_smart(sv[0], req, resp);
		run_write_mock(sv[0], req, resp);
		puts(" OK");
	}

	run_oversized_rejection(sv[0], req, resp);
	puts("oversized rejection... OK");

	close(sv[0]);
	waitpid(child_pid, &status, 0);
	/*
	 * The child's own libnvme_loopback_end() (see child.c) is what
	 * actually verifies every installed mock command was consumed --
	 * this process's copy of *hdl never changes, since only the child's
	 * calls to libnvme_exec_admin_passthru() touch its own forked copy.
	 * An abnormal exit here means that assertion (or another one in the
	 * child) failed.
	 */
	check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
	      "child exited abnormally (status 0x%x)", status);

	libnvme_close(hdl);
	libnvme_free_global_ctx(ctx);
	free(req);
	free(resp);

	puts("privsep-spike: all rounds OK");
	return 0;
}
