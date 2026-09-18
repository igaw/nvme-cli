/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of nvme-cli.
 *
 * privsep helper hardening (issue #3879 Phase 4): capability drop to
 * CAP_SYS_ADMIN and a seccomp-bpf syscall filter. See harden.c for the
 * exact sequence and why it fires once, after the first successful real
 * device open, not at process start.
 */
#pragma once

#include <stddef.h>

/**
 * harden_describe() - Human-readable summary of the target hardening
 *			policy, for security review (issue #3879)
 * @buf: destination buffer
 * @bufsize: size of @buf
 *
 * Describes what harden_once() *will* do (target capability, seccomp
 * allowlist by syscall name, resolved via libseccomp from the same
 * numeric list install_seccomp() actually loads -- not a hand-maintained
 * second copy that could drift), not the process's current state --
 * meaningful even before harden_once() has run, e.g. when reported in
 * the HELLO handshake (issue #3879), which is always the very first
 * message, well before any device is open. Truncates silently (via
 * snprintf's own semantics) if @bufsize is too small; this is a
 * diagnostic aid, not something a caller should size-negotiate over.
 */
void harden_describe(char *buf, size_t bufsize);

/**
 * harden_once() - Drop to CAP_SYS_ADMIN and install the seccomp filter
 *
 * Idempotent: a second call is a no-op. Any failure along the way is
 * fatal (the process exits) -- continuing to serve requests with more
 * privilege than intended would defeat the entire point.
 *
 * Not unit-testable in an unprivileged environment: cap_set_proc() can
 * only shrink a capability the process already holds in its PERMITTED
 * set, so this needs real privilege (root, or a process already holding
 * CAP_SYS_ADMIN) to succeed. See the Phase 4 plan's manual verification
 * step.
 */
void harden_once(void);
