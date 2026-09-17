/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of nvme-cli.
 *
 * privsep helper device-path allowlist (issue #3879 Phase 4). Pure
 * function, deliberately its own translation unit so it's unit-testable
 * (test-allowlist.c) without pulling in the rest of the helper.
 */
#pragma once

#include <stdbool.h>

/**
 * privsep_is_allowed_devname() - Is @name a device the helper may open?
 * @name: the devname string as received from the (untrusted, unprivileged)
 *	  parent over the wire -- never trust it without this check.
 *
 * Accepts exactly:
 *   - "/dev/nvme-fabrics"
 *   - "/dev/nvme<N>" or "/dev/nvme<N>n<M>" or "/dev/ng<N>n<M>", where the
 *     numeric pattern consumes the *entire* remainder of the string --
 *     unlike __libnvme_transport_handle_open_direct()'s own sscanf()
 *     check (lib-linux.c), which only requires a prefix match and so
 *     accepts trailing garbage after the digits.
 *   - the two NVME_TEST_FD/NVME_TEST_FD64 test sentinels -- the same
 *     carve-out libnvme_open() itself already makes unconditionally;
 *     never a real path, never reachable in a real invocation.
 *
 * Return: true if @name may be opened, false otherwise.
 */
bool privsep_is_allowed_devname(const char *name);
