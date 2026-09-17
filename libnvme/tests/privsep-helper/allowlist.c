// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of nvme-cli.
 *
 * See allowlist.h.
 */
#include <string.h>
#include <stdio.h>

#include "allowlist.h"

/*
 * Two helpers, not one shared by argument count: sscanf() consumes
 * varargs positionally according to the format string's own directive
 * count, so a single generic wrapper called with a mismatched number of
 * pointers for a given format (e.g. passing a spare NULL "in case %n
 * needs it") would silently bind %n to the wrong pointer instead of
 * being ignored -- a real NULL-deref risk caught in review here, not
 * hypothetical.
 *
 * Width-limited (%9d, not %d) in both: an unbounded digit run risks int
 * overflow in sscanf(), which is undefined behavior. Nine digits
 * comfortably covers any real device index while keeping the parse
 * itself safe regardless of what a malicious caller sends.
 */
static bool full_match1(const char *s, const char *fmt, int *a)
{
	int consumed = 0;

	return sscanf(s, fmt, a, &consumed) == 1 &&
		s[consumed] == '\0' && *a >= 0;
}

static bool full_match2(const char *s, const char *fmt, int *a, int *b)
{
	int consumed = 0;

	return sscanf(s, fmt, a, b, &consumed) == 2 &&
		s[consumed] == '\0' && *a >= 0 && *b >= 0;
}

bool privsep_is_allowed_devname(const char *name)
{
	int id, ns;

	if (!strcmp(name, "/dev/nvme-fabrics"))
		return true;

	/* Test-only sentinels: libnvme_open() (lib-linux.c) already
	 * special-cases these two literal strings unconditionally. Never a
	 * real path, never reachable from a real invocation.
	 */
	if (!strcmp(name, "NVME_TEST_FD") || !strcmp(name, "NVME_TEST_FD64"))
		return true;

	if (strncmp(name, "/dev/", 5))
		return false;
	name += 5;

	if (full_match2(name, "nvme%9dn%9d%n", &id, &ns))
		return true;
	if (full_match1(name, "nvme%9d%n", &id))
		return true;
	if (full_match2(name, "ng%9dn%9d%n", &id, &ns))
		return true;

	return false;
}
