// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of nvme-cli.
 *
 * Unit tests for ioctl-allowlist.c's privsep_is_allowed_ioctl() (issue
 * #3879 Phase 5). Pure lookup + size check, no privilege needed.
 */
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/sed-opal.h>
#include <stdint.h>
#include <stdio.h>

#include "ioctl-allowlist.h"
#include "sfx-ioctl.h"

/* A real NVMe Admin passthru ioctl number, deliberately not in the
 * allowlist -- confirms this relay can't be used as a side door around
 * the Admin/IO op's own validation.
 */
#define LIBNVME_IOCTL_ADMIN_CMD_TEST _IOWR('N', 0x41, char[72])

static int failures;

static void check(bool got, bool want, const char *what)
{
	if (got != want) {
		fprintf(stderr, "FAIL: %s = %s, expected %s\n", what,
			got ? "true" : "false", want ? "true" : "false");
		failures++;
	}
}

#define ACCEPT(request, size) \
	check(privsep_is_allowed_ioctl((request), (size)), true, #request " with correct size")
#define REJECT_SIZE(request, size) \
	check(privsep_is_allowed_ioctl((request), (size)), false, #request " with wrong size")
#define REJECT_UNKNOWN(request, size) \
	check(privsep_is_allowed_ioctl((request), (size)), false, #request " unknown request")

int main(void)
{
	/* Allowlisted, correct size */
	ACCEPT(IOC_OPAL_TAKE_OWNERSHIP, sizeof(struct opal_key));
	ACCEPT(IOC_OPAL_ACTIVATE_LSP, sizeof(struct opal_lr_act));
	ACCEPT(IOC_OPAL_LR_SETUP, sizeof(struct opal_user_lr_setup));
	ACCEPT(IOC_OPAL_SET_PW, sizeof(struct opal_new_pw));
	ACCEPT(IOC_OPAL_LOCK_UNLOCK, sizeof(struct opal_lock_unlock));
	ACCEPT(IOC_OPAL_REVERT_TPR, sizeof(struct opal_key));
	ACCEPT(IOC_OPAL_DISCOVERY, LIBNVME_PRIVSEP_OPAL_DISCOVERY_BUF_SIZE);
	ACCEPT(BLKRRPART, 0);
	ACCEPT(BLKSSZGET, sizeof(int));
	ACCEPT(BLKGETSIZE64, sizeof(uint64_t));
	ACCEPT(NVME_IOCTL_CLR_CARD, 0);

	/* Allowlisted, wrong size -- the client-lied-about-buffer-size case */
	REJECT_SIZE(IOC_OPAL_TAKE_OWNERSHIP, sizeof(struct opal_key) - 1);
	REJECT_SIZE(IOC_OPAL_TAKE_OWNERSHIP, sizeof(struct opal_key) + 1);
	REJECT_SIZE(IOC_OPAL_LOCK_UNLOCK, 0);
	REJECT_SIZE(BLKRRPART, 4);	/* no-arg ioctl claiming a payload */
	REJECT_SIZE(IOC_OPAL_DISCOVERY, sizeof(struct opal_discovery));
	REJECT_SIZE(NVME_IOCTL_CLR_CARD, sizeof(int));

	/* Not allowlisted at all */
	REJECT_UNKNOWN(LIBNVME_IOCTL_ADMIN_CMD_TEST, 72);
	REJECT_UNKNOWN(BLKFLSBUF, 0);
	REJECT_UNKNOWN(0, 0);
	REJECT_UNKNOWN(0xdeadbeefUL, 4);

	if (failures) {
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}

	puts("privsep-ioctl-allowlist: all cases OK");
	return 0;
}
