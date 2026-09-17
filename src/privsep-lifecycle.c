// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of nvme-cli.
 *
 * nvme-cli process-lifecycle wiring for privilege separation (issue
 * #3879, Phase 3). See privsep-lifecycle.h for the design rationale.
 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <libnvme.h>

#include "privsep-lifecycle.h"

bool privsep_should_engage(uid_t ruid, uid_t euid)
{
	return euid == 0;
}

bool privsep_find_drop_target(uid_t ruid, gid_t rgid, uid_t euid, gid_t egid,
		const char *sudo_uid, const char *sudo_gid,
		uid_t *out_uid, gid_t *out_gid)
{
	if (ruid != euid) {
		*out_uid = ruid;
		*out_gid = rgid;
		return true;
	}

	if (sudo_uid && *sudo_uid && sudo_gid && *sudo_gid) {
		char *end_uid, *end_gid;
		long uid = strtol(sudo_uid, &end_uid, 10);
		long gid = strtol(sudo_gid, &end_gid, 10);

		if (!*end_uid && !*end_gid && uid > 0 && gid > 0) {
			*out_uid = (uid_t)uid;
			*out_gid = (gid_t)gid;
			return true;
		}
	}

	return false;
}

#ifdef CONFIG_PRIVSEP
#include "nvme/privsep.h"
#include "nvme/privsep-proto.h"

#ifndef NVME_PRIVSEP_HELPER_DEFAULT_PATH
#define NVME_PRIVSEP_HELPER_DEFAULT_PATH "/usr/sbin/nvme-privsep-helper"
#endif

#define PRIVSEP_SOCK_FD 3

static struct libnvme_transport_handle *privsep_channel;

static const char *privsep_helper_path(void)
{
	const char *env = getenv("NVME_PRIVSEP_HELPER_PATH");

	return env && *env ? env : NVME_PRIVSEP_HELPER_DEFAULT_PATH;
}

static void privsep_teardown(void)
{
	if (privsep_channel)
		libnvme_close(privsep_channel);
}

struct libnvme_transport_handle *privsep_startup(void)
{
	const char *path;
	uid_t drop_uid = 0;
	gid_t drop_gid = 0;
	bool have_drop_target;
	int sv[2];
	pid_t pid;
	struct libnvme_global_ctx *ctx;

	if (!privsep_should_engage(getuid(), geteuid()))
		return NULL;

	path = privsep_helper_path();
	if (access(path, X_OK) != 0)
		return NULL;

	have_drop_target = privsep_find_drop_target(getuid(), getgid(),
			geteuid(), getegid(),
			getenv("SUDO_UID"), getenv("SUDO_GID"),
			&drop_uid, &drop_gid);

	if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) < 0)
		return NULL;
	libnvme_privsep_size_socket_buffers(sv);

	pid = fork();
	if (pid < 0) {
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	if (pid == 0) {
		char *argv[2] = { (char *)path, NULL };

		/* Same fd-number collision to avoid as the Phase 1/2 test
		 * harness: close sv[0] before dup2()ing sv[1] into
		 * PRIVSEP_SOCK_FD, since socketpair() can hand back sv[0]
		 * as that same number.
		 */
		close(sv[0]);
		if (sv[1] != PRIVSEP_SOCK_FD) {
			if (dup2(sv[1], PRIVSEP_SOCK_FD) < 0)
				_exit(126);
			close(sv[1]);
		}

		execv(path, argv);
		_exit(127);
	}

	close(sv[1]);

	if (have_drop_target) {
		/* Fail closed: if we can't honor the drop, don't continue
		 * privileged with a live helper connection.
		 */
		if (setgid(drop_gid) || setuid(drop_uid)) {
			close(sv[0]);
			waitpid(pid, NULL, 0);
			return NULL;
		}
	}

	ctx = libnvme_create_global_ctx();
	if (!ctx || libnvme_open_privsep(ctx, sv[0], &privsep_channel)) {
		close(sv[0]);
		waitpid(pid, NULL, 0);
		return NULL;
	}

	atexit(privsep_teardown);

	return privsep_channel;
}

struct libnvme_transport_handle *privsep_get_channel(void)
{
	return privsep_channel;
}

#else /* !CONFIG_PRIVSEP */

struct libnvme_transport_handle *privsep_startup(void)
{
	return NULL;
}

struct libnvme_transport_handle *privsep_get_channel(void)
{
	return NULL;
}

#endif
