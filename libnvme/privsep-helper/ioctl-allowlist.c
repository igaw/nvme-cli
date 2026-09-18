// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of nvme-cli.
 *
 * See ioctl-allowlist.h.
 */
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/sed-opal.h>
#include <stdint.h>
#include <stdio.h>

#include "ioctl-allowlist.h"
#include "sfx-ioctl.h"

/*
 * Expected size is tracked explicitly per entry rather than trusted from
 * _IOC_SIZE(request) alone: most of these ioctl numbers encode their
 * argument size correctly and _IOC_SIZE() would agree, but BLKSSZGET is a
 * legacy block-layer ioctl defined via bare _IO() -- no size encoded in
 * the number at all -- despite actually taking an `int *` argument (see
 * sfx-nvme.c). Trusting _IOC_SIZE() there would demand arg_size == 0 and
 * reject every real call. Listing the size explicitly is correct for
 * every entry and self-documenting either way.
 */
struct allowed_ioctl {
	unsigned long request;
	size_t arg_size;
};

static const struct allowed_ioctl allowed_ioctls[] = {
	/* SED-Opal (sedopal_cmd.c), block-layer subsystem, magic 'p' */
	{ IOC_OPAL_TAKE_OWNERSHIP, sizeof(struct opal_key) },
	{ IOC_OPAL_ACTIVATE_LSP, sizeof(struct opal_lr_act) },
	{ IOC_OPAL_LR_SETUP, sizeof(struct opal_user_lr_setup) },
	{ IOC_OPAL_SET_PW, sizeof(struct opal_new_pw) },
	{ IOC_OPAL_LOCK_UNLOCK, sizeof(struct opal_lock_unlock) },
	{ IOC_OPAL_REVERT_TPR, sizeof(struct opal_key) },
#ifdef IOC_OPAL_PSID_REVERT_TPR
	{ IOC_OPAL_PSID_REVERT_TPR, sizeof(struct opal_key) },
#endif
#ifdef IOC_OPAL_REVERT_LSP
	{ IOC_OPAL_REVERT_LSP, sizeof(struct opal_revert_lsp) },
#endif
#ifdef IOC_OPAL_SET_SID_PW
	{ IOC_OPAL_SET_SID_PW, sizeof(struct opal_new_pw) },
#endif
#ifdef IOC_OPAL_DISCOVERY
	/* Not sizeof(struct opal_discovery) -- see the comment on
	 * LIBNVME_PRIVSEP_OPAL_DISCOVERY_BUF_SIZE in the header.
	 */
	{ IOC_OPAL_DISCOVERY, LIBNVME_PRIVSEP_OPAL_DISCOVERY_BUF_SIZE },
#endif
	/* Generic block layer (sedopal_cmd.c and sfx-nvme.c) */
	{ BLKRRPART, 0 },
	{ BLKSSZGET, sizeof(int) },
	{ BLKGETSIZE64, sizeof(uint64_t) },
	/* ScaleFlux-specific (sfx-nvme.c) */
	{ NVME_IOCTL_CLR_CARD, 0 },
};

bool privsep_is_allowed_ioctl(unsigned long request, size_t arg_size)
{
	size_t i;

	for (i = 0; i < sizeof(allowed_ioctls) / sizeof(allowed_ioctls[0]); i++) {
		if (allowed_ioctls[i].request == request)
			return arg_size == allowed_ioctls[i].arg_size;
	}

	return false;
}

void privsep_describe_ioctl_allowlist(char *buf, size_t bufsize)
{
	size_t off;
	size_t i;

	off = (size_t)snprintf(buf, bufsize,
		"raw-ioctl allowlist (%zu entries, request:arg_size in hex):",
		sizeof(allowed_ioctls) / sizeof(allowed_ioctls[0]));
	if (off >= bufsize)
		return;

	for (i = 0; i < sizeof(allowed_ioctls) / sizeof(allowed_ioctls[0]); i++) {
		int n = snprintf(buf + off, bufsize - off, " 0x%lx:0x%zx",
				  allowed_ioctls[i].request, allowed_ioctls[i].arg_size);

		if (n < 0 || (size_t)n >= bufsize - off) {
			off = bufsize;
			break;
		}
		off += (size_t)n;
	}

	if (off < bufsize)
		snprintf(buf + off, bufsize - off, "\n");
}
