// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Phase 1 privsep test harness (issue #3879): drives the real
 * LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP dispatch through a genuinely
 * execve()'d helper process (libnvme/tests/privsep-helper/helper.c)
 * against the NVME_TEST_FD/NVME_TEST_FD64 dry-run backend -- the first
 * thing in this repo to exercise that mechanism. See helper.c's header
 * comment for exactly what this can and cannot validate.
 *
 * Each session spawns a fresh helper, since forcing the 32-bit vs 64-bit
 * ioctl state machine is a per-process ctx setting the helper self-
 * configures before it ever calls libnvme_open() -- this harness cannot
 * reach across the exec boundary to change it afterward.
 *
 * Phase 3: the helper no longer takes a device name via argv (nvme-cli's
 * real usage forks before any argv is parsed, so the device name isn't
 * known at spawn time either) -- this harness now opens it explicitly via
 * LIBNVME_PRIVSEP_OP_OPEN_DEVICE (libnvme_privsep_open_device()) after
 * the helper is already running, matching the real flow.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libnvme.h>

#include "nvme/privsep-proto.h"
#include "nvme/privsep.h"
#include "util.h"

#define PRIVSEP_SOCK_FD 3

static const char *helper_path;

static pid_t spawn_helper(bool force32, int *sock_out)
{
	int sv[2];
	pid_t pid;

	check(!socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv),
	      "socketpair failed: %s", strerror(errno));
	libnvme_privsep_size_socket_buffers(sv);

	pid = fork();
	check(pid >= 0, "fork failed: %s", strerror(errno));

	if (pid == 0) {
		char *argv[3] = { (char *)helper_path, NULL, NULL };

		if (force32)
			argv[1] = (char *)"32";

		/*
		 * Close sv[0] first: socketpair() commonly hands back the two
		 * lowest free fds, so sv[0] can itself land on
		 * PRIVSEP_SOCK_FD. Closing sv[1] into that slot before
		 * closing sv[0] would close the wrong end -- dup2() closes
		 * whatever already occupies its target fd as part of the
		 * dup, so a later close(sv[0]) referring to that same number
		 * would tear down the socket end just set up.
		 */
		close(sv[0]);
		if (sv[1] != PRIVSEP_SOCK_FD) {
			if (dup2(sv[1], PRIVSEP_SOCK_FD) < 0)
				_exit(126);
			close(sv[1]);
		}

		execv(helper_path, argv);
		_exit(127);
	}

	close(sv[1]);
	*sock_out = sv[0];

	return pid;
}

static void reap_helper(pid_t pid)
{
	int status;

	waitpid(pid, &status, 0);
	check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
	      "helper exited abnormally (status 0x%x)", status);
}

static void check_pattern(const uint8_t *data, uint32_t len, uint8_t opcode)
{
	uint32_t i;

	for (i = 0; i < len; i++)
		check(data[i] == (uint8_t)(i ^ opcode),
		      "data[%" PRIu32 "] = 0x%02x, expected 0x%02x",
		      i, data[i], (uint8_t)(i ^ opcode));
}

static void check_result(uint64_t result, uint8_t opcode, const char *what)
{
	check(result == (0xA5A5A5A5u ^ opcode),
	      "%s: unexpected result 0x%" PRIx64, what, result);
}

/*
 * Phase 3: before the first successful OPEN_DEVICE, the helper has no
 * hdl -- libnvme_exec_admin_passthru()'s own `if (!hdl) return -ENODEV;`
 * (ioctl-linux.c) handles this without any special-casing in the helper.
 */
static void run_admin_before_open_rejected(struct libnvme_transport_handle *hdl)
{
	struct nvme_id_ctrl id = {};
	struct libnvme_passthru_cmd cmd = {};

	nvme_init_identify_ctrl(&cmd, &id);
	check(libnvme_exec_admin_passthru(hdl, &cmd) == -ENODEV,
	      "admin passthru before OPEN_DEVICE should be rejected with -ENODEV");
}

static void run_open_device(struct libnvme_transport_handle *hdl)
{
	check(!libnvme_privsep_open_device(hdl, "NVME_TEST_FD", 0),
	      "OPEN_DEVICE(NVME_TEST_FD) failed");
}

static void run_admin_identify(struct libnvme_transport_handle *hdl)
{
	struct nvme_id_ctrl id = {};
	struct libnvme_passthru_cmd cmd = {};

	nvme_init_identify_ctrl(&cmd, &id);
	check(libnvme_exec_admin_passthru(hdl, &cmd) == 0, "identify failed");
	check_result(cmd.result, nvme_admin_identify, "identify");
	check_pattern((const uint8_t *)&id, sizeof(id), nvme_admin_identify);
}

static void run_admin_get_log_smart(struct libnvme_transport_handle *hdl)
{
	struct nvme_smart_log log = {};
	struct libnvme_passthru_cmd cmd = {
		.opcode = nvme_admin_get_log_page,
		.nsid = NVME_NSID_ALL,
		.cdw10 = (uint32_t)(NVME_LOG_LID_SMART |
			(((sizeof(log) >> 2) - 1) << 16)),
		.data_len = sizeof(log),
		.addr = (uint64_t)(uintptr_t)&log,
	};

	check(libnvme_exec_admin_passthru(hdl, &cmd) == 0, "get-log-smart failed");
	check_result(cmd.result, nvme_admin_get_log_page, "get-log-smart");
	check_pattern((const uint8_t *)&log, sizeof(log), nvme_admin_get_log_page);
}

static void run_io_write(struct libnvme_transport_handle *hdl)
{
	uint8_t buf[512] = {};
	struct libnvme_passthru_cmd cmd = {
		.opcode = nvme_cmd_write,
		.nsid = 1,
		.data_len = sizeof(buf),
		.addr = (uint64_t)(uintptr_t)buf,
	};

	check(libnvme_exec_io_passthru(hdl, &cmd) == 0, "io write failed");
	check_result(cmd.result, nvme_cmd_write, "io write");
	check_pattern(buf, sizeof(buf), nvme_cmd_write);
}

/*
 * Phase 2: relay a fabrics connect through the same helper and channel used
 * for admin/IO passthru above. There is no dry-run double for
 * __nvmf_add_ctrl() (unlike NVME_TEST_FD for ioctl passthru), so this can
 * only validate the relay itself -- the real connect fails cleanly here
 * because this machine has no /dev/nvme-fabrics (confirmed absent). On a
 * machine that does have it, the connect would instead fail for a
 * different reason (no real target at this address) -- either way the
 * assertion is just "some negative status came back through the real
 * exec'd helper," which is also exactly what a fabrics-disabled build
 * reports (-ENOTSUP), so one assertion covers both configurations.
 */
static void run_fabrics_connect_relay(struct libnvme_transport_handle *hdl)
{
	static const char argstr[] =
		"transport=tcp,nqn=nqn.2014-08.org.nvmexpress:privsep-spike,"
		"traddr=203.0.113.1,trsvcid=4420";
	int instance = -1;
	int ret = __libnvme_privsep_fabrics_connect(hdl, argstr, &instance);

	check(ret < 0, "fabrics connect relay: expected a negative status "
	      "(no real target), got %d (instance=%d)", ret, instance);
}

/*
 * Bypasses libnvme_open_privsep()/libnvme_exec_admin_passthru() entirely to
 * exercise the shared framing code's (privsep-proto.h) own independent
 * rejection directly -- the same code path the helper relies on, not just
 * the client-side guard in ioctl-privsep.c.
 */
static void run_oversized_rejection(int sock)
{
	struct libnvme_privsep_req *req = malloc(sizeof(*req));
	struct libnvme_privsep_resp *resp = malloc(sizeof(*resp));
	ssize_t n;
	int ret;

	check(req && resp, "out of memory");
	memset(req, 0, offsetof(struct libnvme_privsep_req, data));
	req->op = LIBNVME_PRIVSEP_OP_ADMIN;
	req->opcode = nvme_admin_get_log_page;
	req->data_len = LIBNVME_PRIVSEP_MAX_XFER + 1; /* deliberately invalid */

	n = send(sock, req, offsetof(struct libnvme_privsep_req, data), 0);
	check(n > 0, "oversized: send failed: %s", strerror(errno));

	ret = libnvme_privsep_recv_resp(sock, resp);
	check(ret == 1, "oversized: recv failed (%d)", ret);
	check(resp->status == -EMSGSIZE,
	      "oversized request wasn't rejected with -EMSGSIZE (got %d)",
	      resp->status);

	free(req);
	free(resp);
}

static void run_session(bool force32)
{
	struct libnvme_global_ctx *ctx;
	struct libnvme_transport_handle *hdl;
	int sock;
	pid_t pid = spawn_helper(force32, &sock);

	ctx = libnvme_create_global_ctx();
	check(!libnvme_open_privsep(ctx, sock, &hdl),
	      "libnvme_open_privsep failed");

	run_admin_before_open_rejected(hdl);
	run_open_device(hdl);
	run_admin_identify(hdl);
	run_admin_get_log_smart(hdl);
	run_io_write(hdl);
	run_fabrics_connect_relay(hdl);
	run_oversized_rejection(sock);

	libnvme_close(hdl); /* also closes sock, signaling the helper to exit */
	libnvme_free_global_ctx(ctx);
	reap_helper(pid);
}

int main(int argc, char **argv)
{
	check(argc == 2, "usage: %s <path-to-test-privsep-helper>", argv[0]);
	helper_path = argv[1];

	printf("session: default probing (expect 64-bit ioctl path)...");
	fflush(stdout);
	run_session(false);
	puts(" OK");

	printf("session: probing disabled (expect 32-bit ioctl path)...");
	fflush(stdout);
	run_session(true);
	puts(" OK");

	puts("privsep: all sessions OK");
	return 0;
}
