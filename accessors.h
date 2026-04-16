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
struct libnvmf_uri;

/****************************************************************************
 * Accessors for: struct libnvmf_uri
 ****************************************************************************/

/**
 * libnvmf_uri_set_path_segments() - Set path_segments.
 * @p: The &struct libnvmf_uri instance to update.
 * @path_segments: New NULL-terminated string array; deep-copied.
 */
void libnvmf_uri_set_path_segments(
		struct libnvmf_uri *p,
		const char *const *path_segments);

/**
 * libnvmf_uri_get_path_segments() - Get path_segments.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the path_segments field.
 */
const char *const *libnvmf_uri_get_path_segments(const struct libnvmf_uri *p);

#endif /* _ACCESSORS_H_ */
