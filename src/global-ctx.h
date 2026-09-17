/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of nvme-cli.
 * Copyright (c) 2026 SUSE Software Solutions
 *
 * Authors: Daniel Wagner <dwagner@suse.de>
 */
#pragma once

#include <stddef.h>

struct libnvme_global_ctx;
struct libnvme_transport_handle;
struct argconfig_commandline_options;

void put_transport_handle(struct libnvme_transport_handle *hdl);

/*
 * nvme_raw_ioctl() - issue a raw (non-passthru) ioctl on @hdl's device,
 * transparently to whichever transport backs @hdl.
 * @hdl: an already-open transport handle
 * @request: the ioctl request number
 * @arg: argument buffer (may be NULL for a no-arg ioctl)
 * @arg_size: size of @arg -- for a PRIVSEP handle this must match what
 *            the helper's fixed ioctl allowlist expects for @request, or
 *            the helper refuses the request
 *
 * For a PRIVSEP handle, relays @request through the helper's allowlisted
 * raw-ioctl channel instead of issuing it directly -- there is no local
 * fd to ioctl() in that case (libnvme_transport_handle_get_fd() returns
 * -1). For every other handle type, this is the same ioctl() these
 * callers (plugins/sed, plugins/scaleflux) already issued directly.
 *
 * Special case: for IOC_OPAL_DISCOVERY, @arg/@arg_size is the actual
 * discovery buffer, not struct opal_discovery (linux/sed-opal.h) -- that
 * struct's `data` field is a pointer the kernel dereferences directly,
 * which can't cross the PRIVSEP process boundary. This function builds
 * the real kernel argument internally for both transports, so callers
 * never construct struct opal_discovery themselves.
 *
 * Return: 0 on success, a negative errno on failure -- normalized the
 * same way regardless of transport, unlike a bare ioctl() call (which
 * returns -1 and sets the caller's errno, a convention that can't cross
 * the PRIVSEP process boundary).
 */
int nvme_raw_ioctl(struct libnvme_transport_handle *hdl, unsigned long request,
		void *arg, size_t arg_size);

/*
 * nvme_create_global_ctx_hostnqn() - Create context and resolve host identity
 * @ctx: output global context
 * @hostnqn_arg: optional hostnqn override
 * @hostid_arg: optional hostid override
 * @hostnqn: optional output resolved hostnqn (caller owns/frees when provided)
 * @hostid: optional output resolved hostid (caller owns/frees when provided)
 *
 * Creates a global context, applies --set-options, resolves hostnqn/hostid
 * via libnvmf_host_get_ids(), and stores the resolved values in the context.
 * This function has to be called after @parse_args.
 */
int nvme_create_global_ctx_hostnqn(struct libnvme_global_ctx **ctx,
		const char *hostnqn_arg, const char *hostid_arg,
		char **hostnqn, char **hostid);

int nvme_create_global_ctx(struct libnvme_global_ctx **ctx);

/*
 * parse_and_open - parses arguments and opens the NVMe device, populating @ctx, @hdl
 */
int parse_and_open(struct libnvme_global_ctx **ctx,
		struct libnvme_transport_handle **hdl, int argc, char **argv,
		const char *desc, struct argconfig_commandline_options *clo);

int open_exclusive(struct libnvme_global_ctx **ctx,
		struct libnvme_transport_handle **hdl, int argc, char **argv,
		int ignore_exclusive, struct argconfig_commandline_options *opts);
