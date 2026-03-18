# SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * This file is part of libnvme.
 *
 * Copyright (c) 2025, Dell Technologies Inc. or its subsidiaries.
 * Authors: Martin Belanger <Martin.Belanger@dell.com>
 *
 */

LIBNVMF_ACCESSORS_3 {
	global:
		libnvmf_context_get_ctrlkey;
		libnvmf_context_get_default_keep_alive_timeout;
		libnvmf_context_get_default_max_discovery_retries;
		libnvmf_context_get_device;
		libnvmf_context_get_host_iface;
		libnvmf_context_get_host_traddr;
		libnvmf_context_get_hostid;
		libnvmf_context_get_hostkey;
		libnvmf_context_get_hostnqn;
		libnvmf_context_get_keyring;
		libnvmf_context_get_persistent;
		libnvmf_context_get_subsysnqn;
		libnvmf_context_get_tls_key_identity;
		libnvmf_context_get_traddr;
		libnvmf_context_get_transport;
		libnvmf_context_get_trsvcid;
		libnvmf_context_set_default_keep_alive_timeout;
		libnvmf_context_set_default_max_discovery_retries;
		libnvmf_context_set_device;
		libnvmf_context_set_persistent;
		libnvmf_discovery_args_free;
		libnvmf_discovery_args_get_lsp;
		libnvmf_discovery_args_get_max_retries;
		libnvmf_discovery_args_init_defaults;
		libnvmf_discovery_args_new;
		libnvmf_discovery_args_set_lsp;
		libnvmf_discovery_args_set_max_retries;
		libnvmf_uri_get_fragment;
		libnvmf_uri_get_host;
		libnvmf_uri_get_path_segments;
		libnvmf_uri_get_port;
		libnvmf_uri_get_protocol;
		libnvmf_uri_get_query;
		libnvmf_uri_get_scheme;
		libnvmf_uri_get_userinfo;
		libnvmf_uri_set_fragment;
		libnvmf_uri_set_host;
		libnvmf_uri_set_path_segments;
		libnvmf_uri_set_port;
		libnvmf_uri_set_protocol;
		libnvmf_uri_set_query;
		libnvmf_uri_set_scheme;
		libnvmf_uri_set_userinfo;
};
