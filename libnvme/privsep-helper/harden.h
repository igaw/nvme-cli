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
