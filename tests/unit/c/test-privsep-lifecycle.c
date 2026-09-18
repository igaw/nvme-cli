// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of nvme-cli.
 *
 * Unit tests for src/privsep-lifecycle.c's pure decision functions
 * (issue #3879 Phase 3). These take uid/gid values and env-var strings as
 * plain arguments specifically so the interesting decision table can be
 * covered without the test process itself needing to be root -- the real
 * fork/exec/setuid mechanics in privsep_startup() need a manual check on
 * a machine where that's possible instead.
 */
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include <shared/assert-util.h>

#include "../src/privsep-lifecycle.h"

static void run_test(const char *name, void (*fn)(void))
{
	printf("Running test %s...", name);
	fflush(stdout);
	fn();
	puts(" OK");
}

#define RUN_TEST(name) run_test(#name, test_##name)

static void test_should_engage(void)
{
	shr_assert(privsep_should_engage(0, 0));
	shr_assert(privsep_should_engage(1000, 0));	/* setuid binary */
	shr_assert(!privsep_should_engage(0, 1000));	/* can't happen, but not root */
	shr_assert(!privsep_should_engage(1000, 1000));
}

static void test_drop_target_setuid(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* real != effective: classic setuid binary, drop to the real ids,
	 * regardless of anything SUDO_UID/SUDO_GID might claim.
	 */
	shr_assert(privsep_find_drop_target(1000, 2000, 0, 0,
			NULL, NULL, "3000", "4000", &uid, &gid));
	shr_assert(uid == 1000);
	shr_assert(gid == 2000);
}

static void test_drop_target_sudo(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* real == effective (both 0): sudo fully switches identity, so the
	 * SUDO_UID/SUDO_GID convention is the only way to find who invoked.
	 */
	shr_assert(privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, "1000", "1000", &uid, &gid));
	shr_assert(uid == 1000);
	shr_assert(gid == 1000);
}

static void test_drop_target_sudo_uid_zero_rejected(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* SUDO_UID=0 isn't a real drop target -- defensively reject it
	 * rather than "drop to root".
	 */
	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, "0", "0", &uid, &gid));
}

static void test_drop_target_sudo_malformed_rejected(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, "not-a-number", "1000", &uid, &gid));
	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, "1000", NULL, &uid, &gid));
	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, NULL, NULL, &uid, &gid));
}

static void test_drop_target_bare_root_no_target(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* A bare root shell: real == effective, no sudo env vars, no
	 * explicit override. Honest "nothing to drop to", not a crash or a
	 * guessed identity.
	 */
	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			NULL, NULL, NULL, NULL, &uid, &gid));
}

static void test_drop_target_explicit_override(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* NVME_PRIVSEP_DROP_UID/GID on an otherwise bare-root process (no
	 * setuid bit, no sudo) -- exactly the case an orchestrator with no
	 * natural non-root identity of its own needs.
	 */
	shr_assert(privsep_find_drop_target(0, 0, 0, 0,
			"5000", "5000", NULL, NULL, &uid, &gid));
	shr_assert(uid == 5000);
	shr_assert(gid == 5000);
}

static void test_drop_target_explicit_override_wins_priority(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* The explicit override outranks both auto-detected sources, not
	 * just the bare-root fallback.
	 */
	shr_assert(privsep_find_drop_target(1000, 2000, 0, 0,
			"5000", "5000", NULL, NULL, &uid, &gid));
	shr_assert(uid == 5000);
	shr_assert(gid == 5000);

	uid = 12345;
	gid = 12345;
	shr_assert(privsep_find_drop_target(0, 0, 0, 0,
			"5000", "5000", "3000", "4000", &uid, &gid));
	shr_assert(uid == 5000);
	shr_assert(gid == 5000);
}

static void test_drop_target_explicit_override_zero_rejected(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* Same "drop to root isn't a drop" rule applies to an explicit
	 * request as to SUDO_UID=0 -- falls through, doesn't silently
	 * grant it. Bare root here, so it falls all the way through to
	 * "no target found".
	 */
	shr_assert(!privsep_find_drop_target(0, 0, 0, 0,
			"0", "0", NULL, NULL, &uid, &gid));
}

static void test_drop_target_explicit_override_malformed_falls_through(void)
{
	uid_t uid = 12345;
	gid_t gid = 12345;

	/* A malformed override doesn't hard-error -- it falls through to
	 * the next priority level, here the setuid case. */
	shr_assert(privsep_find_drop_target(1000, 2000, 0, 0,
			"not-a-number", "5000", NULL, NULL, &uid, &gid));
	shr_assert(uid == 1000);
	shr_assert(gid == 2000);
}

int main(void)
{
	RUN_TEST(should_engage);
	RUN_TEST(drop_target_setuid);
	RUN_TEST(drop_target_sudo);
	RUN_TEST(drop_target_sudo_uid_zero_rejected);
	RUN_TEST(drop_target_sudo_malformed_rejected);
	RUN_TEST(drop_target_bare_root_no_target);
	RUN_TEST(drop_target_explicit_override);
	RUN_TEST(drop_target_explicit_override_wins_priority);
	RUN_TEST(drop_target_explicit_override_zero_rejected);
	RUN_TEST(drop_target_explicit_override_malformed_falls_through);

	return 0;
}
