/* SPDX-License-Identifier: LGPL-2.1-or-later */

/*
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
#ifndef _ACCESSORS_FABRICS_H_
#define _ACCESSORS_FABRICS_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <nvme/types.h>

/* Forward declarations. These are internal (opaque) structs. */
struct libnvmf_context;
struct libnvmf_discovery_args;
struct libnvmf_uri;

/****************************************************************************
 * Accessors for: struct libnvmf_context
 ****************************************************************************/

/**
 * libnvmf_context_set_default_max_discovery_retries() - Setter.
 * @p: The &struct libnvmf_context instance to update.
 * @default_max_discovery_retries: Value to assign to the default_max_discovery_retries field.
 */
void libnvmf_context_set_default_max_discovery_retries(
		struct libnvmf_context *p,
		int default_max_discovery_retries);

/**
 * libnvmf_context_get_default_max_discovery_retries() - Getter.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the default_max_discovery_retries field.
 */
int libnvmf_context_get_default_max_discovery_retries(
		const struct libnvmf_context *p);

/**
 * libnvmf_context_set_default_keep_alive_timeout() - Setter.
 * @p: The &struct libnvmf_context instance to update.
 * @default_keep_alive_timeout: Value to assign to the default_keep_alive_timeout field.
 */
void libnvmf_context_set_default_keep_alive_timeout(
		struct libnvmf_context *p,
		int default_keep_alive_timeout);

/**
 * libnvmf_context_get_default_keep_alive_timeout() - Getter.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the default_keep_alive_timeout field.
 */
int libnvmf_context_get_default_keep_alive_timeout(
		const struct libnvmf_context *p);

/**
 * libnvmf_context_set_device() - Set device.
 * @p: The &struct libnvmf_context instance to update.
 * @device: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_context_set_device(struct libnvmf_context *p, const char *device);

/**
 * libnvmf_context_get_device() - Get device.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the device field, or NULL if not set.
 */
const char *libnvmf_context_get_device(const struct libnvmf_context *p);

/**
 * libnvmf_context_set_persistent() - Set persistent.
 * @p: The &struct libnvmf_context instance to update.
 * @persistent: Value to assign to the persistent field.
 */
void libnvmf_context_set_persistent(struct libnvmf_context *p, bool persistent);

/**
 * libnvmf_context_get_persistent() - Get persistent.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the persistent field.
 */
bool libnvmf_context_get_persistent(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_subsysnqn() - Get subsysnqn.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the subsysnqn field, or NULL if not set.
 */
const char *libnvmf_context_get_subsysnqn(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_transport() - Get transport.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the transport field, or NULL if not set.
 */
const char *libnvmf_context_get_transport(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_traddr() - Get traddr.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the traddr field, or NULL if not set.
 */
const char *libnvmf_context_get_traddr(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_trsvcid() - Get trsvcid.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the trsvcid field, or NULL if not set.
 */
const char *libnvmf_context_get_trsvcid(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_host_traddr() - Get host_traddr.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the host_traddr field, or NULL if not set.
 */
const char *libnvmf_context_get_host_traddr(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_host_iface() - Get host_iface.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the host_iface field, or NULL if not set.
 */
const char *libnvmf_context_get_host_iface(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_hostnqn() - Get hostnqn.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the hostnqn field, or NULL if not set.
 */
const char *libnvmf_context_get_hostnqn(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_hostid() - Get hostid.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the hostid field, or NULL if not set.
 */
const char *libnvmf_context_get_hostid(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_hostkey() - Get hostkey.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the hostkey field, or NULL if not set.
 */
const char *libnvmf_context_get_hostkey(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_ctrlkey() - Get ctrlkey.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the ctrlkey field, or NULL if not set.
 */
const char *libnvmf_context_get_ctrlkey(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_keyring() - Get keyring.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the keyring field, or NULL if not set.
 */
const char *libnvmf_context_get_keyring(const struct libnvmf_context *p);

/**
 * libnvmf_context_get_tls_key_identity() - Get tls_key_identity.
 * @p: The &struct libnvmf_context instance to query.
 *
 * Return: The value of the tls_key_identity field, or NULL if not set.
 */
const char *libnvmf_context_get_tls_key_identity(
		const struct libnvmf_context *p);

/****************************************************************************
 * Accessors for: struct libnvmf_discovery_args
 ****************************************************************************/

/**
 * libnvmf_discovery_args_new() - Allocate and initialise a new instance.
 * @pp: On success, *pp is set to the newly allocated object.
 *
 * Allocates a zeroed &struct libnvmf_discovery_args on the heap.
 * The caller must release it with libnvmf_discovery_args_free().
 *
 * Return: 0 on success, -EINVAL if @pp is NULL,
 *         -ENOMEM if allocation fails.
 */
int libnvmf_discovery_args_new(struct libnvmf_discovery_args **pp);

/**
 * libnvmf_discovery_args_free() - Release a libnvmf_discovery_args object.
 * @p: Object previously returned by libnvmf_discovery_args_new().
 *     A NULL pointer is silently ignored.
 */
void libnvmf_discovery_args_free(struct libnvmf_discovery_args *p);

/**
 * libnvmf_discovery_args_init_defaults() - Set fields to their defaults.
 * @p: The &struct libnvmf_discovery_args instance to initialise.
 *
 * Sets each field that carries a default annotation to its
 * compile-time default value.  Called automatically by
 * libnvmf_discovery_args_new() but may also be called directly to reset an
 * instance to its defaults without reallocating it.
 */
void libnvmf_discovery_args_init_defaults(struct libnvmf_discovery_args *p);

/**
 * libnvmf_discovery_args_set_max_retries() - Set max_retries.
 * @p: The &struct libnvmf_discovery_args instance to update.
 * @max_retries: Value to assign to the max_retries field.
 */
void libnvmf_discovery_args_set_max_retries(
		struct libnvmf_discovery_args *p,
		int max_retries);

/**
 * libnvmf_discovery_args_get_max_retries() - Get max_retries.
 * @p: The &struct libnvmf_discovery_args instance to query.
 *
 * Return: The value of the max_retries field.
 */
int libnvmf_discovery_args_get_max_retries(
		const struct libnvmf_discovery_args *p);

/**
 * libnvmf_discovery_args_set_lsp() - Set lsp.
 * @p: The &struct libnvmf_discovery_args instance to update.
 * @lsp: Value to assign to the lsp field.
 */
void libnvmf_discovery_args_set_lsp(struct libnvmf_discovery_args *p, __u8 lsp);

/**
 * libnvmf_discovery_args_get_lsp() - Get lsp.
 * @p: The &struct libnvmf_discovery_args instance to query.
 *
 * Return: The value of the lsp field.
 */
__u8 libnvmf_discovery_args_get_lsp(const struct libnvmf_discovery_args *p);

/****************************************************************************
 * Accessors for: struct libnvmf_uri
 ****************************************************************************/

/**
 * libnvmf_uri_set_scheme() - Set scheme.
 * @p: The &struct libnvmf_uri instance to update.
 * @scheme: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_scheme(struct libnvmf_uri *p, const char *scheme);

/**
 * libnvmf_uri_get_scheme() - Get scheme.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the scheme field, or NULL if not set.
 */
const char *libnvmf_uri_get_scheme(const struct libnvmf_uri *p);

/**
 * libnvmf_uri_set_protocol() - Set protocol.
 * @p: The &struct libnvmf_uri instance to update.
 * @protocol: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_protocol(struct libnvmf_uri *p, const char *protocol);

/**
 * libnvmf_uri_get_protocol() - Get protocol.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the protocol field, or NULL if not set.
 */
const char *libnvmf_uri_get_protocol(const struct libnvmf_uri *p);

/**
 * libnvmf_uri_set_userinfo() - Set userinfo.
 * @p: The &struct libnvmf_uri instance to update.
 * @userinfo: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_userinfo(struct libnvmf_uri *p, const char *userinfo);

/**
 * libnvmf_uri_get_userinfo() - Get userinfo.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the userinfo field, or NULL if not set.
 */
const char *libnvmf_uri_get_userinfo(const struct libnvmf_uri *p);

/**
 * libnvmf_uri_set_host() - Set host.
 * @p: The &struct libnvmf_uri instance to update.
 * @host: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_host(struct libnvmf_uri *p, const char *host);

/**
 * libnvmf_uri_get_host() - Get host.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the host field, or NULL if not set.
 */
const char *libnvmf_uri_get_host(const struct libnvmf_uri *p);

/**
 * libnvmf_uri_set_port() - Set port.
 * @p: The &struct libnvmf_uri instance to update.
 * @port: Value to assign to the port field.
 */
void libnvmf_uri_set_port(struct libnvmf_uri *p, int port);

/**
 * libnvmf_uri_get_port() - Get port.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the port field.
 */
int libnvmf_uri_get_port(const struct libnvmf_uri *p);

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

/**
 * libnvmf_uri_set_query() - Set query.
 * @p: The &struct libnvmf_uri instance to update.
 * @query: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_query(struct libnvmf_uri *p, const char *query);

/**
 * libnvmf_uri_get_query() - Get query.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the query field, or NULL if not set.
 */
const char *libnvmf_uri_get_query(const struct libnvmf_uri *p);

/**
 * libnvmf_uri_set_fragment() - Set fragment.
 * @p: The &struct libnvmf_uri instance to update.
 * @fragment: New string; a copy is stored. Pass NULL to clear.
 */
void libnvmf_uri_set_fragment(struct libnvmf_uri *p, const char *fragment);

/**
 * libnvmf_uri_get_fragment() - Get fragment.
 * @p: The &struct libnvmf_uri instance to query.
 *
 * Return: The value of the fragment field, or NULL if not set.
 */
const char *libnvmf_uri_get_fragment(const struct libnvmf_uri *p);

#endif /* _ACCESSORS_FABRICS_H_ */
