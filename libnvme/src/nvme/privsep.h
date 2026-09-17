/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libnvme.
 *
 * LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP (issue #3879, Phase 1): a passthru
 * channel to a helper process that issues the real ioctl. Not yet part of
 * the public <libnvme.h> umbrella header -- like loopback.h, this is
 * included directly by whoever needs it (currently: the Phase 1 test
 * harness and helper binary). Phase 3 will decide the real public/opt-in
 * activation shape (mirroring libnvme_set_owner()-style setters) once
 * process-lifecycle spawning exists; this signature is transitional.
 */
#pragma once

struct libnvme_global_ctx;
struct libnvme_transport_handle;
struct libnvme_passthru_cmd;

/**
 * libnvme_open_privsep() - Wrap an already-connected socket in a PRIVSEP
 *			    transport handle
 * @ctx: Library context
 * @sock: A connected SOCK_SEQPACKET socket to a helper process speaking
 *	  the libnvme_privsep_req/resp protocol (privsep-proto.h). The
 *	  handle takes ownership: libnvme_close() closes it.
 * @hdlp: On success, set to the new transport handle
 *
 * Does not spawn a helper: the caller (in Phase 1, the test harness; in a
 * later phase, nvme-cli's own process-lifecycle code) is responsible for
 * forking, executing the helper binary, and connecting @sock before
 * calling this.
 *
 * Return: 0 on success, negative error code otherwise (e.g. -ENOTSUP if
 * the library was built without privsep support).
 */
int libnvme_open_privsep(struct libnvme_global_ctx *ctx, int sock,
		struct libnvme_transport_handle **hdlp);

int __libnvme_privsep_admin_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd);
int __libnvme_privsep_io_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd);

/* Closes hdl->privsep_sock and frees hdl. Called from libnvme_close(); does
 * not reap a helper process, since this phase does not spawn one.
 */
void __libnvme_privsep_close(struct libnvme_transport_handle *hdl);
