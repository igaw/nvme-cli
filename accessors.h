/* SPDX-License-Identifier: LGPL-2.1-or-later */

/**
 * This file is part of libnvme.
 *
 * Copyright (c) 2025, Dell Technologies Inc. or its subsidiaries.
 * Authors: Martin Belanger <Martin.Belanger@dell.com>
 *
 *   ____                           _           _    ____          _
 *  / ___| ___ _ __   ___ _ __ __ _| |_ ___  __| |  / ___|___   __| | ___
 * | |  _ / _ \ '_ \ / _ \ '__/ _` | __/ _ \/ _` | | |   / _ \ / _` |/ _ \
 * | |_| |  __/ | | |  __/ | | (_| | ||  __/ (_| | | |__| (_) | (_| |  __/
 *  \____|\___|_| |_|\___|_|  \__,_|\__\___|\__,_|  \____\___/ \__,_|\___|
 *
 * Auto-generated struct member accessors (setter/getter)
 *
 * To update run: meson compile -C [BUILD-DIR] update-accessors
 * Or:            make update-accessors
 */
#ifndef _ACCESSORS_H_
#define _ACCESSORS_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <nvme/types.h>

/* Forward declarations. These are internal (opaque) structs. */
struct libnvme_fabrics_uri;

/****************************************************************************
 * Accessors for: struct libnvme_fabrics_uri
 ****************************************************************************/

/**
 * libnvme_fabrics_uri_set_scheme() - Set scheme.
 * @p: The &struct libnvme_fabrics_uri instance to update.
 * @scheme: New string; a copy is stored. Pass NULL to clear.
 */
void libnvme_fabrics_uri_set_scheme(
		struct libnvme_fabrics_uri *p,
		const char *scheme);

/**
 * libnvme_fabrics_uri_get_scheme() - Get scheme.
 * @p: The &struct libnvme_fabrics_uri instance to query.
 *
 * Return: The value of the scheme field, or NULL if not set.
 */
const char *libnvme_fabrics_uri_get_scheme(const struct libnvme_fabrics_uri *p);

/**
 * libnvme_fabrics_uri_set_port() - Set port.
 * @p: The &struct libnvme_fabrics_uri instance to update.
 * @port: Value to assign to the port field.
 */
void libnvme_fabrics_uri_set_port(struct libnvme_fabrics_uri *p, int port);

/**
 * libnvme_fabrics_uri_get_port() - Get port.
 * @p: The &struct libnvme_fabrics_uri instance to query.
 *
 * Return: The value of the port field.
 */
int libnvme_fabrics_uri_get_port(const struct libnvme_fabrics_uri *p);

/**
 * libnvme_fabrics_uri_set_path_segments() - Set path_segments.
 * @p: The &struct libnvme_fabrics_uri instance to update.
 * @path_segments: New NULL-terminated string array; deep-copied.
 */
void libnvme_fabrics_uri_set_path_segments(
		struct libnvme_fabrics_uri *p,
		const char *const *path_segments);

/**
 * libnvme_fabrics_uri_get_path_segments() - Get path_segments.
 * @p: The &struct libnvme_fabrics_uri instance to query.
 *
 * Return: The value of the path_segments field.
 */
const char *const *libnvme_fabrics_uri_get_path_segments(
		const struct libnvme_fabrics_uri *p);

#endif /* _ACCESSORS_H_ */
