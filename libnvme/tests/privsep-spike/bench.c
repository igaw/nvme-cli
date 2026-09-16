// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Phase-0 privsep spike (issue #3879): IPC overhead measurement.
 *
 * Sweeps the synthetic write-direction mock command across payload sizes
 * from a small Identify-like size up to SPIKE_MAX_XFER (telemetry-scale),
 * over a fresh fork+socketpair per size, and reports round-trip latency
 * and throughput. Informational only -- answers the issue's "per-command
 * IPC overhead" and "buffer size cap for large transfers" risks with real
 * numbers instead of a guess; there is no pass/fail gate here.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <libnvme.h>

#include "nvme/loopback.h"
#include "proto.h"
#include "util.h"

struct sweep_point {
	size_t size;
	int iters;
};

static const struct sweep_point sweep[] = {
	{ 64,               10000 },
	{ 4096,             10000 },
	{ 64 * 1024,         2000 },
	{ 256 * 1024,         500 },
	{ 1024 * 1024,        200 },
	{ SPIKE_MAX_XFER,      50 },
};

static int64_t now_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static int cmp_i64(const void *a, const void *b)
{
	int64_t x = *(const int64_t *)a, y = *(const int64_t *)b;

	return (x > y) - (x < y);
}

static void run_one_size(size_t size, int iters)
{
	struct libnvme_global_ctx *ctx;
	struct libnvme_transport_handle *hdl;
	struct libnvme_loopback_cmd *cmds;
	uint8_t *in_data = malloc(size);
	struct privsep_req *req = malloc(sizeof(*req));
	struct privsep_resp *resp = malloc(sizeof(*resp));
	int64_t *lat = malloc(sizeof(*lat) * (size_t)iters);
	int sv[2];
	pid_t child_pid;
	int i, status;
	int64_t total_ns = 0;

	check(in_data && req && resp && lat, "out of memory");
	arbitrary(in_data, size);

	cmds = calloc((size_t)iters, sizeof(*cmds));
	check(cmds, "out of memory");
	for (i = 0; i < iters; i++)
		cmds[i] = (struct libnvme_loopback_cmd){
			.opcode = SPIKE_MOCK_WRITE_OPCODE,
			.data_len = (uint32_t)size,
			.in_data = in_data,
		};

	ctx = libnvme_create_global_ctx();
	check(!libnvme_open_loopback(ctx, &hdl), "opening loopback failed");
	libnvme_loopback_set_admin_cmds(hdl, cmds, (size_t)iters);

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

	memset(req, 0, offsetof(struct privsep_req, data));
	req->opcode = SPIKE_MOCK_WRITE_OPCODE;
	req->data_len = (uint32_t)size;
	memcpy(req->data, in_data, size);

	for (i = 0; i < iters; i++) {
		int64_t t0 = now_ns();
		int ret;

		check(privsep_send_req(sv[0], req) == (ssize_t)privsep_req_len(req),
		      "send failed: %s", strerror(errno));
		ret = privsep_recv_resp(sv[0], resp);
		check(ret == 1, "recv failed or rejected (%d)", ret);
		check(resp->status == 0, "child reported status %d", resp->status);

		lat[i] = now_ns() - t0;
		total_ns += lat[i];
	}

	close(sv[0]);
	waitpid(child_pid, &status, 0);
	/* See child.c: the child's own libnvme_loopback_end() (run right
	 * before it exits) is what actually verifies consumption -- this
	 * process's copy of *hdl never changes.
	 */
	check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
	      "child exited abnormally (status 0x%x)", status);
	libnvme_close(hdl);
	libnvme_free_global_ctx(ctx);

	qsort(lat, (size_t)iters, sizeof(*lat), cmp_i64);

	{
		int median_idx = iters / 2;
		int p99_idx = (iters * 99) / 100;
		double mb = (double)size / (1024.0 * 1024.0);
		double avg_us = (double)total_ns / iters / 1000.0;
		double mbps = total_ns ?
			mb * iters / ((double)total_ns / 1e9) : 0.0;

		printf("size=%8zu  iters=%6d  min=%8.2fus  median=%8.2fus  "
		       "p99=%8.2fus  avg=%8.2fus  throughput=%8.2f MB/s\n",
		       size, iters,
		       lat[0] / 1000.0,
		       lat[median_idx] / 1000.0,
		       lat[p99_idx] / 1000.0,
		       avg_us, mbps);
	}

	free(in_data);
	free(req);
	free(resp);
	free(lat);
	free(cmds);
}

int main(void)
{
	size_t i;

	for (i = 0; i < sizeof(sweep) / sizeof(sweep[0]); i++)
		run_one_size(sweep[i].size, sweep[i].iters);

	return 0;
}
