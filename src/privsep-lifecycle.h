/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of nvme-cli.
 *
 * nvme-cli process-lifecycle wiring for privilege separation (issue
 * #3879, Phase 3). Always compiled: when built without CONFIG_PRIVSEP,
 * privsep_startup()/privsep_get_channel() are harmless no-ops (NULL),
 * so callers (main(), get_transport_handle() in src/global-ctx.c) never
 * need their own #ifdef.
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

struct libnvme_transport_handle;

/**
 * privsep_should_engage() - Should privsep engage for this process?
 * @ruid: real uid
 * @euid: effective uid
 *
 * Privsep exists to confine an already-privileged process -- a bug in
 * argument/config-ini parsing must not run as root -- not to elevate an
 * unprivileged one. Engage whenever the process currently holds root,
 * full stop; not tied to any capability introspection of the helper.
 *
 * Pure function, deliberately taking its inputs as plain arguments (not
 * calling getuid()/geteuid() itself) so it's unit-testable without the
 * test process needing to actually be root.
 */
bool privsep_should_engage(uid_t ruid, uid_t euid);

/**
 * privsep_find_drop_target() - Find the identity the parent should drop
 *				 to after forking
 * @ruid, @rgid, @euid, @egid: the process's real/effective ids
 * @sudo_uid, @sudo_gid: the SUDO_UID/SUDO_GID environment values, or NULL
 * @out_uid, @out_gid: on success, the identity to drop to
 *
 * Priority order:
 *   1. real != effective (classic setuid binary) -> drop to real uid/gid.
 *   2. else SUDO_UID/SUDO_GID present, parse as valid positive integers
 *      -> drop to those (the standard `sudo` convention). SUDO_UID=0 is
 *      rejected defensively -- "drop to root" isn't a drop.
 *   3. else: no distinguishable lower-privilege identity exists (a bare
 *      root shell, no sudo, no setuid bit) -- returns false. Privsep
 *      still engages in this case (see privsep_should_engage()), but the
 *      parent cannot usefully drop; the confinement benefit on the
 *      parent side is zero here, though the child is still confined and
 *      Phase 4's hardening still applies to it.
 *
 * Pure function, unit-testable without real uids/env vars.
 *
 * Return: true if a drop target was found (and written to @out_uid/
 * @out_gid), false otherwise.
 */
bool privsep_find_drop_target(uid_t ruid, gid_t rgid, uid_t euid, gid_t egid,
		const char *sudo_uid, const char *sudo_gid,
		uid_t *out_uid, gid_t *out_gid);

/**
 * privsep_startup() - Engage privsep for this process, if appropriate
 *
 * Real orchestration (issue #3879 Phase 3), called once at the very top
 * of main(), before any argv or config-ini parsing: if
 * privsep_should_engage() says yes and a helper binary can be found
 * (NVME_PRIVSEP_HELPER_PATH environment override, else a compiled-in
 * default path -- nothing installs a real one there before Phase 6),
 * forks, execs the helper over a fresh socketpair, and in the parent
 * drops to the identity found by privsep_find_drop_target() (if any) and
 * wraps the connected socket via libnvme_open_privsep(). Registers its
 * own atexit() teardown.
 *
 * Known limitation, not solved here: the resulting channel's own
 * libnvme_global_ctx is independent of whatever ctx each command later
 * creates via parse_and_open() (nvme-cli creates a fresh one per
 * command) -- flags set on that per-command ctx, like --dry-run, do not
 * reach the helper's ctx, since dry_run is only ever consulted on the
 * real ioctl path inside the helper process.
 *
 * Return: the shared channel handle (also retrievable afterward via
 * privsep_get_channel()), or NULL if privsep is not in use for this
 * invocation -- callers fall back to the existing direct libnvme_open()
 * path unchanged.
 *
 * @argc, @argv: passed through untouched from main(), before any real
 * argconfig parsing -- used only for a best-effort verbosity pre-scan
 * (-v/-vv/-vvv/--verbose/-q/--quiet) so this function's own lifecycle
 * narration (and the wire-level narration in ioctl-privsep.c, via the
 * channel's ctx) can honor -vv from the very first line, before
 * nvme_args.verbose exists. Not a real argconfig parse and not
 * authoritative -- degrades gracefully if it misses something, since the
 * real parse (later, per command) sets log_level again regardless.
 */
struct libnvme_transport_handle *privsep_startup(int argc, char **argv);

/**
 * privsep_get_channel() - The channel set up by privsep_startup()
 *
 * Return: the channel handle, or NULL if privsep isn't active for this
 * process (including when built without CONFIG_PRIVSEP). Used by
 * get_transport_handle() (src/global-ctx.c) to decide which path to take.
 */
struct libnvme_transport_handle *privsep_get_channel(void);
