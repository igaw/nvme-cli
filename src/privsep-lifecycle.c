// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of nvme-cli.
 *
 * nvme-cli process-lifecycle wiring for privilege separation (issue
 * #3879, Phase 3). See privsep-lifecycle.h for the design rationale.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/*
 * The real build always defines this (root meson.build, -Dprivsep=true:
 * sbindir / 'nvme-privsep-helper', matching where
 * libnvme/privsep-helper/meson.build actually installs it -- issue
 * #3879 Phase 6). This fallback only matters for a hand-invoked compiler
 * command outside the normal build.
 */
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

/*
 * This file's own narration level, independent of nvme-cli's shared
 * log_level (src/logging.c): privsep_startup() runs at the very top of
 * main(), before any argv is parsed, so it can't know the eventual
 * command's --output-format and must not write to stdout the way
 * print_debug() does -- that could corrupt a later `-o json` invocation.
 * Narrated here on stderr instead, gated by this file-local level.
 */
static int privsep_log_level;

static void privsep_log(int level, const char *fmt, ...)
{
	va_list ap;

	if (privsep_log_level < level)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*
 * A deliberate local copy of src/logging.c's map_log_level(), not a call
 * to it: pulling that file in (even just for this one function) would
 * drag this translation unit's link step through nvme-cli's entire
 * display stack (nvme-print.c and its json-c dependency) via
 * nvme_submit_entry()/_exit(), for a process-lifecycle file that has no
 * business depending on any of that. Keep in sync by hand if logging.c's
 * mapping ever changes.
 */
static int privsep_map_log_level(int verbose, bool quiet)
{
	if (quiet)
		return LIBNVME_LOG_ERR;

	switch (verbose) {
	case 0:
		return LIBNVME_LOG_ERR;
	case 1:
		return LIBNVME_LOG_INFO;
	case 2:
		return LIBNVME_LOG_DEBUG;
	default:
		break;
	}

	return LIBNVME_LOG_DEBUG_VERBOSE;
}

/*
 * Best-effort, not a real argconfig parse -- see privsep_startup()'s doc
 * comment. Counts -v/--verbose occurrences (including combined short-opt
 * clusters like -vv) and -q/--quiet, the same shape argconfig's own
 * OPT_INCR/OPT_FLAG handling gives nvme_args.verbose/.quiet, but without
 * needing the command table argconfig itself only builds per-command,
 * later. Stops at a bare "--" (end of options).
 */
static void privsep_prescan_verbosity(int argc, char **argv, int *verbose,
		bool *quiet)
{
	int v = 0;
	bool q = false;
	int i;

	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		const char *p;

		if (!strcmp(arg, "--"))
			break;
		if (!strcmp(arg, "--verbose")) {
			v++;
			continue;
		}
		if (!strcmp(arg, "--quiet")) {
			q = true;
			continue;
		}
		if (arg[0] != '-' || arg[1] == '-' || arg[1] == '\0')
			continue;

		for (p = arg + 1; *p; p++) {
			if (*p == 'v')
				v++;
			else if (*p == 'q')
				q = true;
		}
	}

	*verbose = v;
	*quiet = q;
}

struct libnvme_transport_handle *privsep_startup(int argc, char **argv)
{
	const char *path;
	uid_t drop_uid = 0;
	gid_t drop_gid = 0;
	bool have_drop_target;
	int sv[2];
	pid_t pid;
	struct libnvme_global_ctx *ctx;
	int verbose;
	bool quiet;

	privsep_prescan_verbosity(argc, argv, &verbose, &quiet);
	privsep_log_level = privsep_map_log_level(verbose, quiet);

	if (!privsep_should_engage(getuid(), geteuid())) {
		privsep_log(LIBNVME_LOG_DEBUG, "privsep: not engaging (euid %d != 0)\n",
			    geteuid());
		return NULL;
	}

	path = privsep_helper_path();
	privsep_log(LIBNVME_LOG_DEBUG,
		    "privsep: engaging (euid 0); checking helper at '%s'\n", path);
	if (access(path, X_OK) != 0) {
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: helper not found or not executable at "
			    "'%s', falling back to direct ioctl\n", path);
		return NULL;
	}

	have_drop_target = privsep_find_drop_target(getuid(), getgid(),
			geteuid(), getegid(),
			getenv("SUDO_UID"), getenv("SUDO_GID"),
			&drop_uid, &drop_gid);
	if (have_drop_target)
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: parent will drop to uid=%d gid=%d "
			    "after fork\n", drop_uid, drop_gid);
	else
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: no drop target found (bare root, no "
			    "setuid bit, no SUDO_UID/SUDO_GID); parent stays "
			    "root, child is still confined\n");

	if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) < 0) {
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: socketpair() failed, falling back to "
			    "direct ioctl\n");
		return NULL;
	}
	libnvme_privsep_size_socket_buffers(sv);

	pid = fork();
	if (pid < 0) {
		privsep_log(LIBNVME_LOG_WARN, "privsep: fork: %s\n", strerror(errno));
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	if (pid == 0) {
		char *child_argv[2] = { (char *)path, NULL };

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

		execv(path, child_argv);
		_exit(127);
	}

	privsep_log(LIBNVME_LOG_DEBUG, "privsep: forked helper pid %d\n", (int)pid);
	close(sv[1]);

	if (have_drop_target) {
		/* Fail closed: if we can't honor the drop, don't continue
		 * privileged with a live helper connection.
		 */
		if (setgid(drop_gid) || setuid(drop_uid)) {
			privsep_log(LIBNVME_LOG_WARN,
				    "privsep: setuid/setgid to drop target: %s\n",
				    strerror(errno));
			close(sv[0]);
			waitpid(pid, NULL, 0);
			return NULL;
		}
		privsep_log(LIBNVME_LOG_DEBUG, "privsep: parent dropped to uid=%d "
			    "gid=%d\n", getuid(), getgid());
	}

	ctx = libnvme_create_global_ctx();
	if (ctx) {
		libnvme_set_logging_file(ctx, stderr);
		libnvme_set_logging_level(ctx, privsep_log_level, false, false);
	}
	if (!ctx || libnvme_open_privsep(ctx, sv[0], &privsep_channel)) {
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: failed to establish channel with helper, "
			    "falling back to direct ioctl\n");
		close(sv[0]);
		waitpid(pid, NULL, 0);
		return NULL;
	}

	privsep_log(LIBNVME_LOG_DEBUG, "privsep: channel established\n");
	atexit(privsep_teardown);

	return privsep_channel;
}

struct libnvme_transport_handle *privsep_get_channel(void)
{
	return privsep_channel;
}

#else /* !CONFIG_PRIVSEP */

struct libnvme_transport_handle *privsep_startup(int argc, char **argv)
{
	return NULL;
}

struct libnvme_transport_handle *privsep_get_channel(void)
{
	return NULL;
}

#endif
