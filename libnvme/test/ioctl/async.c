// SPDX-License-Identifier: LGPL-2.1-or-later

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <libnvme.h>

#include "mock.h"
#include "util.h"

static struct libnvme_transport_handle *test_hdl;

static void test_async_submit_not_supported(void)
{
	struct nvme_id_ctrl id = {};
	struct libnvme_passthru_cmd cmd;
	int err;

	nvme_init_identify_ctrl(&cmd, &id);
	err = libnvme_submit_admin_passthru_async(test_hdl, &cmd,
			(void *)(uintptr_t)0x1234);
	check(err == -ENOTSUP, "submit async returned %d, expected %d",
	      err, -ENOTSUP);
}

static void test_async_reap_not_supported(void)
{
	struct libnvme_admin_passthru_completion completion = {};
	int err;

	err = libnvme_reap_admin_passthru_async(test_hdl, &completion);
	check(err == -ENOTSUP, "reap async returned %d, expected %d",
	      err, -ENOTSUP);
}

static void test_sync_path_fallback(void)
{
	struct nvme_id_ctrl expected_id = {}, id = {};
	struct mock_cmd mock_admin_cmd = {
		.opcode = nvme_admin_identify,
		.data_len = sizeof(expected_id),
		.cdw10 = NVME_IDENTIFY_CNS_CTRL,
		.out_data = &expected_id,
	};
	struct libnvme_passthru_cmd cmd;
	int err;

	arbitrary(&expected_id, sizeof(expected_id));
	set_mock_admin_cmds(&mock_admin_cmd, 1);
	nvme_init_identify_ctrl(&cmd, &id);
	err = libnvme_exec_admin_passthru(test_hdl, &cmd);
	end_mock_cmds();
	check(err == 0, "sync fallback returned %d", err);
	cmp(&id, &expected_id, sizeof(id), "incorrect identify data");
}

static void run_test(const char *test_name, void (*test_fn)(void))
{
	printf("Running test %s...", test_name);
	fflush(stdout);
	test_fn();
	puts(" OK");
}

#define RUN_TEST(name) run_test(#name, test_ ## name)

int main(void)
{
	struct libnvme_global_ctx *ctx =
		libnvme_create_global_ctx(stdout, LIBNVME_DEFAULT_LOGLEVEL);

	set_mock_fd(LIBNVME_TEST_FD);
	check(!libnvme_open(ctx, "NVME_TEST_FD", &test_hdl),
	      "opening test link failed");

	RUN_TEST(async_submit_not_supported);
	RUN_TEST(async_reap_not_supported);
	RUN_TEST(sync_path_fallback);

	libnvme_free_global_ctx(ctx);
}
