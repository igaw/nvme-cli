// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of nvme-cli.
 *
 * Unit tests for allowlist.c's privsep_is_allowed_devname() (issue #3879
 * Phase 4). Pure string matching, no privilege needed -- unlike the
 * capability/seccomp drop, this runs in any environment.
 */
#include <stdio.h>

#include "allowlist.h"

static int failures;

static void check(bool got, bool want, const char *name)
{
	if (got != want) {
		fprintf(stderr, "FAIL: privsep_is_allowed_devname(\"%s\") "
			"= %s, expected %s\n", name,
			got ? "true" : "false", want ? "true" : "false");
		failures++;
	}
}

#define ACCEPT(name) check(privsep_is_allowed_devname(name), true, name)
#define REJECT(name) check(privsep_is_allowed_devname(name), false, name)

int main(void)
{
	/* Real device shapes */
	ACCEPT("/dev/nvme0");
	ACCEPT("/dev/nvme0n1");
	ACCEPT("/dev/nvme12n34");
	ACCEPT("/dev/ng0n1");
	ACCEPT("/dev/ng12n34");
	ACCEPT("/dev/nvme-fabrics");

	/* Test-only sentinels */
	ACCEPT("NVME_TEST_FD");
	ACCEPT("NVME_TEST_FD64");

	/* Trailing garbage after otherwise-valid digits -- the exact laxness
	 * the existing DIRECT-path sscanf() check (lib-linux.c) has, which
	 * this stricter check must not repeat.
	 */
	REJECT("/dev/nvme0n1garbage");
	REJECT("/dev/nvme0n1 ");
	REJECT("/dev/nvme0 extra");
	REJECT("/dev/nvme0n1;rm -rf /");

	/* Path traversal */
	REJECT("/dev/nvme0n1/../../etc/passwd");
	REJECT("/dev/../etc/passwd");
	REJECT("/dev/nvme0/../../../etc/shadow");

	/* Wrong prefix / shape */
	REJECT("/dev/sda");
	REJECT("/dev/nvme");
	REJECT("/dev/nvmex");
	REJECT("/dev/ng0");		/* ng requires the n<M> suffix */
	REJECT("nvme0n1");		/* missing /dev/ */
	REJECT("dev/nvme0n1");
	REJECT("/etc/passwd");
	REJECT("/dev/nvme-fabricsX");
	REJECT("/dev/");
	REJECT("/dev/nvme0n");
	REJECT("");

	/* Defensively-rejected numeric shapes */
	REJECT("/dev/nvme-1n2");	/* negative */
	REJECT("/dev/nvme0n-2");	/* negative */
	REJECT("/dev/nvme9999999999n1"); /* would overflow an unbounded %d */

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}

	puts("privsep-allowlist: all cases OK");
	return 0;
}
