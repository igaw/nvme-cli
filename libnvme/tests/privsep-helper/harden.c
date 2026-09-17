// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of nvme-cli.
 *
 * See harden.h.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/capability.h>
#include <seccomp.h>

#include "harden.h"

static void harden_fatal(const char *what)
{
	fprintf(stderr, "privsep helper: hardening failed at %s, exiting\n", what);
	_exit(125);
}

/*
 * Bounding-set drop first (needs CAP_SETPCAP, still held at this point),
 * then cap_set_proc() shrinks EFFECTIVE/PERMITTED to exactly
 * {CAP_SYS_ADMIN} last. Order matters: dropping EFFECTIVE/PERMITTED
 * first would lose CAP_SETPCAP before the bounding-set loop runs.
 */
static void drop_capabilities(void)
{
	cap_value_t keep = CAP_SYS_ADMIN;
	cap_value_t cap;
	cap_t caps;

	/* Best-effort per bit: an unsupported/unrecognized capability
	 * number on this kernel isn't itself a hardening failure. Only a
	 * real cap_set_proc() failure below is fatal.
	 */
	for (cap = 0; cap < cap_max_bits(); cap++) {
		if (cap != CAP_SYS_ADMIN)
			cap_drop_bound(cap);
	}

	caps = cap_init();
	if (!caps)
		harden_fatal("cap_init");

	if (cap_clear(caps) ||
	    cap_set_flag(caps, CAP_EFFECTIVE, 1, &keep, CAP_SET) ||
	    cap_set_flag(caps, CAP_PERMITTED, 1, &keep, CAP_SET) ||
	    cap_set_proc(caps)) {
		cap_free(caps);
		harden_fatal("cap_set_proc");
	}

	cap_free(caps);
}

/*
 * Starting allowlist for the request-serving loop's actual needs --
 * verified (and corrected, if wrong) by running the real thing and
 * watching for SIGSYS, not just reasoned through: ioctl (the whole
 * point), read/write and the recv/send syscalls proto.h's wrappers use,
 * close, open/openat (repeated device opens across one session --
 * userspace's allowlist.c is the real path check, seccomp can't inspect
 * string arguments), brk/mmap/munmap/mprotect (malloc/free -- libnvme_open()
 * and friends allocate on every call, not just once), fstat/newfstatat,
 * rt_sigreturn, exit/exit_group.
 */
static const int allowed_syscalls[] = {
	SCMP_SYS(ioctl),
	SCMP_SYS(read),
	SCMP_SYS(write),
	SCMP_SYS(recvfrom),
	SCMP_SYS(sendto),
	SCMP_SYS(recvmsg),
	SCMP_SYS(sendmsg),
	SCMP_SYS(close),
	SCMP_SYS(open),
	SCMP_SYS(openat),
	SCMP_SYS(brk),
	SCMP_SYS(mmap),
	SCMP_SYS(munmap),
	SCMP_SYS(mprotect),
	SCMP_SYS(fstat),
	SCMP_SYS(newfstatat),
	SCMP_SYS(rt_sigreturn),
	SCMP_SYS(exit),
	SCMP_SYS(exit_group),
};

static void install_seccomp(void)
{
	scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL_PROCESS);
	size_t i;

	if (!ctx)
		harden_fatal("seccomp_init");

	for (i = 0; i < sizeof(allowed_syscalls) / sizeof(allowed_syscalls[0]); i++) {
		if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, allowed_syscalls[i], 0)) {
			seccomp_release(ctx);
			harden_fatal("seccomp_rule_add");
		}
	}

	if (seccomp_load(ctx)) {
		seccomp_release(ctx);
		harden_fatal("seccomp_load");
	}

	seccomp_release(ctx);
}

void harden_once(void)
{
	static bool hardened;

	if (hardened)
		return;

	drop_capabilities();
	install_seccomp();

	hardened = true;
}
