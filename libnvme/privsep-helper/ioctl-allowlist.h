/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of nvme-cli.
 *
 * privsep helper raw-ioctl allowlist (issue #3879 Phase 5). Pure
 * function, its own translation unit so it's unit-testable
 * (test-ioctl-allowlist.c) without pulling in the rest of the helper --
 * mirrors allowlist.c's device-path pattern.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>

/*
 * IOC_OPAL_DISCOVERY doesn't carry its argument inline like every other
 * allowlisted ioctl -- struct opal_discovery (linux/sed-opal.h) embeds a
 * `data` pointer the kernel dereferences directly, meaningless across
 * the helper's process boundary (it would point into the unprivileged
 * parent's address space, not the helper's). The relay special-cases
 * this one request: the wire payload is the actual discovery buffer,
 * matching sedopal_cmd.c's level0_discovery_buf[4096] byte-for-byte, not
 * struct opal_discovery itself -- see ioctl-allowlist.c's table entry
 * and helper.c's handle_raw_ioctl().
 */
#define LIBNVME_PRIVSEP_OPAL_DISCOVERY_BUF_SIZE 4096

/**
 * privsep_is_allowed_ioctl() - May the helper relay this ioctl?
 * @request: the ioctl request number, as received from the (untrusted,
 *	     unprivileged) parent over the wire -- never trust it without
 *	     this check.
 * @arg_size: the argument buffer size the caller sent.
 *
 * Not a generic "run any ioctl" relay: @request must be exactly one of a
 * fixed, hardcoded set (the SED-Opal block-layer ioctls sedopal_cmd.c
 * needs, plus the handful of generic block/vendor ioctls sfx-nvme.c
 * needs). @arg_size must also exactly equal that request's expected
 * argument size, so a caller can't lie about buffer size for an
 * otherwise-allowed request. The expected size is tracked explicitly per
 * entry rather than derived from _IOC_SIZE(request): most of these
 * numbers do encode their size correctly, but BLKSSZGET is a legacy
 * block-layer ioctl defined via bare _IO() (no size encoded at all)
 * despite actually taking an `int *` argument -- trusting _IOC_SIZE()
 * there would reject every real call.
 *
 * Return: true if the helper may relay this (request, arg_size) pair.
 */
bool privsep_is_allowed_ioctl(unsigned long request, size_t arg_size);
