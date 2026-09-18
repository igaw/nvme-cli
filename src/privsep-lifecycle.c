// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of nvme-cli.
 *
 * nvme-cli process-lifecycle wiring for privilege separation (issue
 * #3879, Phase 3). See privsep-lifecycle.h for the design rationale.
 */
#include <errno.h>
#include <limits.h>
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

/*
 * Shared by the NVME_PRIVSEP_DROP_UID/GID and SUDO_UID/GID cases: both
 * are "a pair of decimal strings from the environment, or nothing" with
 * identical validation (both present, both fully-numeric, both
 * strictly positive -- 0 is never a real drop target, "drop to root"
 * isn't a drop).
 */
static bool parse_uid_gid_pair(const char *uid_str, const char *gid_str,
		uid_t *out_uid, gid_t *out_gid)
{
	char *end_uid, *end_gid;
	long uid, gid;

	if (!uid_str || !*uid_str || !gid_str || !*gid_str)
		return false;

	uid = strtol(uid_str, &end_uid, 10);
	gid = strtol(gid_str, &end_gid, 10);

	if (*end_uid || *end_gid || uid <= 0 || gid <= 0)
		return false;

	*out_uid = (uid_t)uid;
	*out_gid = (gid_t)gid;
	return true;
}

bool privsep_find_drop_target(uid_t ruid, gid_t rgid, uid_t euid, gid_t egid,
		const char *drop_uid_env, const char *drop_gid_env,
		const char *sudo_uid, const char *sudo_gid,
		uid_t *out_uid, gid_t *out_gid)
{
	if (parse_uid_gid_pair(drop_uid_env, drop_gid_env, out_uid, out_gid))
		return true;

	if (ruid != euid) {
		*out_uid = ruid;
		*out_gid = rgid;
		return true;
	}

	if (parse_uid_gid_pair(sudo_uid, sudo_gid, out_uid, out_gid))
		return true;

	return false;
}

#ifdef CONFIG_PRIVSEP
#include <sys/capability.h>

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

/* Defined below, after privsep_log() -- see there for why. */
static const char *privsep_helper_path(void);

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
 * Best-effort discovery of a helper binary next to this process's own
 * real executable path, so the common nvme-cli dev workflow -- run
 * straight out of a meson build directory (./build/nvme,
 * ./.build-ci/nvme, ...), never `meson install`ed -- finds a working
 * helper without NVME_PRIVSEP_HELPER_PATH having to be set by hand
 * every time. /proc/self/exe always resolves to this binary's real
 * absolute path regardless of how it was invoked (relative, via PATH,
 * through a symlink); Linux-only, same constraint privsep already has
 * throughout (seccomp, capabilities).
 *
 * Tries, in order:
 *   1. a sibling of this binary -- the installed layout, where
 *      sbindir/nvme and sbindir/nvme-privsep-helper sit side by side.
 *      Also makes a relocated install (different --prefix than this
 *      binary was compiled with) resolve correctly, for free.
 *   2. libnvme/privsep-helper/<name>, relative to this binary's
 *      directory -- the build-tree layout, where 'nvme' itself lands
 *      at the build root and libnvme/privsep-helper/meson.build's
 *      output sits at that fixed relative path underneath it.
 *
 * Returns @buf (populated) on success, NULL if /proc/self/exe can't be
 * resolved or neither candidate is executable -- caller falls back to
 * NVME_PRIVSEP_HELPER_DEFAULT_PATH.
 */
static const char *privsep_helper_path_relative_to_self(char *buf, size_t bufsize)
{
	static const char *const candidates[] = {
		"nvme-privsep-helper",
		"libnvme/privsep-helper/nvme-privsep-helper",
	};
	char exe[PATH_MAX];
	ssize_t n;
	char *slash;
	size_t i;

	n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
	if (n <= 0)
		return NULL;
	exe[n] = '\0';

	slash = strrchr(exe, '/');
	if (!slash)
		return NULL;
	*slash = '\0'; /* exe now holds this binary's own directory */

	for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
		int len = snprintf(buf, bufsize, "%s/%s", exe, candidates[i]);

		if (len < 0 || (size_t)len >= bufsize)
			continue;

		privsep_log(LIBNVME_LOG_DEBUG, "privsep: trying helper at '%s'\n", buf);
		if (access(buf, X_OK) == 0)
			return buf;
	}

	return NULL;
}

static const char *privsep_helper_path(void)
{
	static char path_buf[PATH_MAX];
	const char *env = getenv("NVME_PRIVSEP_HELPER_PATH");
	const char *found;

	if (env && *env)
		return env;

	found = privsep_helper_path_relative_to_self(path_buf, sizeof(path_buf));
	if (found)
		return found;

	privsep_log(LIBNVME_LOG_DEBUG,
		    "privsep: no helper found relative to this binary's own "
		    "path, falling back to the compiled-in default\n");

	return NVME_PRIVSEP_HELPER_DEFAULT_PATH;
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

/*
 * Always run, unconditionally, before any setuid()/setgid() below --
 * regardless of whether the parent goes on to drop to a real uid or
 * stays uid 0. setuid() to a real non-root uid already clears the
 * process's EFFECTIVE/PERMITTED capability sets as a kernel side effect
 * (capabilities(7), "Effect of user ID changes on capabilities"), but
 * that leaves the BOUNDING set untouched -- no uid transition affects
 * it, only an explicit prctl(PR_CAPBSET_DROP) while CAP_SETPCAP is still
 * held. Left full, anything this (now unprivileged-looking) process
 * later exec()s that's setuid-root or carries file capabilities could
 * still reacquire up to whatever's in that bounding set. Dropping it
 * unconditionally closes that regardless of which uid path is taken.
 *
 * Must run *before* any setuid()/setgid() call, not after: it needs
 * CAP_SETPCAP, which a uid transition away from 0 would already have
 * cleared from EFFECTIVE. Mirrors libnvme/privsep-helper/harden.c's
 * drop_capabilities() bounding-set loop and its same "best-effort per
 * bit" stance -- an unsupported/unrecognized capability number on this
 * kernel isn't itself a hardening failure, matching harden.c's precedent
 * of only fail-closing on the EFFECTIVE/PERMITTED clear that follows
 * (here, in privsep_clear_own_capabilities() below), not on this loop.
 */
static void privsep_drop_capability_bounding_set(void)
{
	cap_value_t cap;

	for (cap = 0; cap < cap_max_bits(); cap++)
		cap_drop_bound(cap);
}

/*
 * Bare-root case only: privsep_find_drop_target() found no real/sudo/
 * explicit identity to setuid()/setgid() away from (a literal root
 * shell, no sudo, no NVME_PRIVSEP_DROP_UID/GID) -- without this, the
 * parent (all the untrusted argv/config-ini parsing and JSON/plugin
 * decode logic privsep exists to confine) would simply stay full root
 * for the rest of the run, defeating the point even though the helper
 * is correctly confined. A real setuid()/setgid() call would already
 * clear EFFECTIVE/PERMITTED as a kernel side effect (see
 * privsep_drop_capability_bounding_set()'s comment); this is the
 * substitute for the case where there's no other uid to become, so
 * nothing does that clearing for us.
 *
 * Drops to EMPTY, not {CAP_SYS_ADMIN} like
 * libnvme/privsep-helper/harden.c's drop_capabilities() (which this
 * mirrors) -- the parent issues no ioctls at all once privsep is
 * engaged, so it needs no capability whatsoever.
 *
 * Return: true on success. A failure here is treated the same as a
 * failed setuid()/setgid() (see privsep_startup()) -- fail closed, no
 * live privileged connection without it.
 */
static bool privsep_clear_own_capabilities(void)
{
	cap_t caps;
	bool ok;

	caps = cap_init();
	if (!caps) {
		privsep_log(LIBNVME_LOG_WARN,
			    "privsep: cap_init failed, can't drop parent capabilities\n");
		return false;
	}

	ok = !cap_clear(caps) && !cap_set_proc(caps);
	if (!ok)
		privsep_log(LIBNVME_LOG_WARN,
			    "privsep: cap_set_proc failed, can't drop parent "
			    "capabilities: %s\n", strerror(errno));
	else
		privsep_log(LIBNVME_LOG_DEBUG, "privsep: parent capabilities dropped\n");

	cap_free(caps);
	return ok;
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
			getenv("NVME_PRIVSEP_DROP_UID"), getenv("NVME_PRIVSEP_DROP_GID"),
			getenv("SUDO_UID"), getenv("SUDO_GID"),
			&drop_uid, &drop_gid);
	if (have_drop_target)
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: parent will drop to uid=%d gid=%d "
			    "after fork\n", drop_uid, drop_gid);
	else
		privsep_log(LIBNVME_LOG_DEBUG,
			    "privsep: no drop target found (bare root, no "
			    "setuid bit, no SUDO_UID/SUDO_GID); parent will "
			    "drop its own capabilities instead after fork\n");

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

	/* Unconditional, and before any setuid()/setgid() below -- see
	 * privsep_drop_capability_bounding_set()'s comment for why both.
	 */
	privsep_drop_capability_bounding_set();

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
	} else if (!privsep_clear_own_capabilities()) {
		/* Same fail-closed rule as above. */
		close(sv[0]);
		waitpid(pid, NULL, 0);
		return NULL;
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
