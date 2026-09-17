// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libnvme.
 */
#include <errno.h>
#include <stdlib.h>

#include <libnvme.h>

#include "privsep.h"

int libnvme_open_privsep(struct libnvme_global_ctx *ctx, int sock,
		struct libnvme_transport_handle **hdlp)
{
	return -ENOTSUP;
}

int __libnvme_privsep_admin_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd)
{
	return -ENOTSUP;
}

int __libnvme_privsep_io_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd)
{
	return -ENOTSUP;
}

int __libnvme_privsep_fabrics_connect(struct libnvme_transport_handle *hdl,
		const char *argstr, int *instance)
{
	return -ENOTSUP;
}

void __libnvme_privsep_close(struct libnvme_transport_handle *hdl)
{
	free(hdl);
}
