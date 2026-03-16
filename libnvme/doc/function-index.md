<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# libnvme API Function Index

Quick-reference table of all public functions exported by libnvme, grouped by
header file.

---

## tree.h — libnvme tree object interface

| Function | Suggested Name |
|---|---|
| `nvme_set_application` | — |
| `nvme_get_application` | — |
| `nvme_skip_namespaces` | — |
| `nvme_release_fds` | — |
| `nvme_first_host` | — |
| `nvme_next_host` | — |
| `nvme_host_get_global_ctx` | `nvme_get_host_global_ctx` |
| `nvme_host_set_pdc_enabled` | `nvme_set_host_pdc_enabled` |
| `nvme_host_is_pdc_enabled` | `nvme_is_host_pdc_enabled` |
| `nvme_host_get` | `nvme_get_host` |
| `nvme_host_get_ids` | `nvme_get_host_ids` |
| `nvme_first_subsystem` | — |
| `nvme_next_subsystem` | — |
| `nvme_subsystem_get` | `nvme_get_subsystem` |
| `nvme_free_subsystem` | — |
| `nvme_subsystem_get_host` | `nvme_get_subsystem_host` |
| `nvme_ctrl_first_ns` | `nvme_first_ctrl_ns` |
| `nvme_ctrl_next_ns` | `nvme_next_ctrl_ns` |
| `nvme_ctrl_first_path` | `nvme_first_ctrl_path` |
| `nvme_ctrl_next_path` | `nvme_next_ctrl_path` |
| `nvme_subsystem_first_ctrl` | `nvme_first_subsystem_ctrl` |
| `nvme_subsystem_next_ctrl` | `nvme_next_subsystem_ctrl` |
| `nvme_namespace_first_path` | `nvme_first_namespace_path` |
| `nvme_namespace_next_path` | `nvme_next_namespace_path` |
| `nvme_ctrl_config_match` | `nvme_match_ctrl_config` |
| `nvme_create_ctrl` | — |
| `nvme_subsystem_first_ns` | `nvme_first_subsystem_ns` |
| `nvme_subsystem_next_ns` | `nvme_next_subsystem_ns` |
| `nvme_for_each_host_safe` | — |
| `nvme_for_each_host` | — |
| `nvme_for_each_subsystem_safe` | — |
| `nvme_for_each_subsystem` | — |
| `nvme_subsystem_for_each_ctrl_safe` | `nvme_for_each_subsystem_ctrl_safe` |
| `nvme_subsystem_for_each_ctrl` | `nvme_for_each_subsystem_ctrl` |
| `nvme_ctrl_for_each_ns_safe` | `nvme_for_each_ctrl_ns_safe` |
| `nvme_ctrl_for_each_ns` | `nvme_for_each_ctrl_ns` |
| `nvme_ctrl_for_each_path_safe` | `nvme_for_each_ctrl_path_safe` |
| `nvme_ctrl_for_each_path` | `nvme_for_each_ctrl_path` |
| `nvme_subsystem_for_each_ns_safe` | `nvme_for_each_subsystem_ns_safe` |
| `nvme_subsystem_for_each_ns` | `nvme_for_each_subsystem_ns` |
| `nvme_namespace_for_each_path_safe` | `nvme_for_each_namespace_path_safe` |
| `nvme_namespace_for_each_path` | `nvme_for_each_namespace_path` |
| `nvme_ns_get_csi` | `nvme_get_ns_csi` |
| `nvme_ns_get_eui64` | `nvme_get_ns_eui64` |
| `nvme_ns_get_nguid` | `nvme_get_ns_nguid` |
| `nvme_ns_get_uuid` | `nvme_get_ns_uuid` |
| `nvme_ns_get_generic_name` | `nvme_get_ns_generic_name` |
| `nvme_ns_get_firmware` | `nvme_get_ns_firmware` |
| `nvme_ns_get_serial` | `nvme_get_ns_serial` |
| `nvme_ns_get_model` | `nvme_get_ns_model` |
| `nvme_ns_get_subsystem` | `nvme_get_ns_subsystem` |
| `nvme_ns_get_ctrl` | `nvme_get_ns_ctrl` |
| `nvme_free_ns` | — |
| `nvme_ns_read` | `nvme_read_ns` |
| `nvme_ns_write` | `nvme_write_ns` |
| `nvme_ns_verify` | `nvme_verify_ns` |
| `nvme_ns_compare` | `nvme_compare_ns` |
| `nvme_ns_write_zeros` | `nvme_write_ns_zeros` |
| `nvme_ns_write_uncorrectable` | `nvme_write_ns_uncorrectable` |
| `nvme_ns_flush` | `nvme_flush_ns` |
| `nvme_ns_identify` | `nvme_identify_ns` |
| `nvme_ns_identify_descs` | `nvme_identify_ns_descs` |
| `nvme_path_get_queue_depth` | `nvme_get_path_queue_depth` |
| `nvme_path_get_ctrl` | `nvme_get_path_ctrl` |
| `nvme_path_get_ns` | `nvme_get_path_ns` |
| `nvme_ctrl_get_transport_handle` | `nvme_get_ctrl_transport_handle` |
| `nvme_ctrl_release_transport_handle` | `nvme_release_ctrl_transport_handle` |
| `nvme_ctrl_get_address` | `nvme_get_ctrl_address` |
| `nvme_ctrl_get_src_addr` | `nvme_get_ctrl_src_addr` |
| `nvme_ctrl_get_phy_slot` | `nvme_get_ctrl_phy_slot` |
| `nvme_ctrl_get_state` | `nvme_get_ctrl_state` |
| `nvme_ctrl_get_subsysnqn` | `nvme_get_ctrl_subsysnqn` |
| `nvme_ctrl_get_subsystem` | `nvme_get_ctrl_subsystem` |
| `nvme_ctrl_get_dhchap_host_key` | `nvme_get_ctrl_dhchap_host_key` |
| `nvme_ctrl_set_dhchap_host_key` | `nvme_set_ctrl_dhchap_host_key` |
| `nvme_ns_head_get_sysfs_dir` | `nvme_get_ns_head_sysfs_dir` |
| `nvme_ctrl_get_config` | `nvme_get_ctrl_config` |
| `nvme_ctrl_is_discovered` | `nvme_is_ctrl_discovered` |
| `nvme_ctrl_is_persistent` | `nvme_is_ctrl_persistent` |
| `nvme_ctrl_is_discovery_ctrl` | `nvme_is_ctrl_discovery_ctrl` |
| `nvme_ctrl_is_unique_discovery_ctrl` | `nvme_is_ctrl_unique_discovery_ctrl` |
| `nvme_ctrl_identify` | `nvme_identify_ctrl` |
| `nvme_disconnect_ctrl` | — |
| `nvme_scan_ctrl` | — |
| `nvme_rescan_ctrl` | — |
| `nvme_init_ctrl` | — |
| `nvme_free_ctrl` | — |
| `nvme_unlink_ctrl` | — |
| `nvme_subsystem_get_nqn` | `nvme_get_subsystem_nqn` |
| `nvme_subsystem_get_type` | `nvme_get_subsystem_type` |
| `nvme_subsystem_get_fw_rev` | `nvme_get_subsystem_fw_rev` |
| `nvme_scan_topology` | — |
| `nvme_host_release_fds` | `nvme_release_host_fds` |
| `nvme_free_host` | — |
| `nvme_read_config` | — |
| `nvme_refresh_topology` | — |
| `nvme_dump_config` | — |
| `nvme_dump_tree` | — |
| `nvme_get_attr` | — |
| `nvme_get_subsys_attr` | — |
| `nvme_get_ctrl_attr` | — |
| `nvme_get_ns_attr` | — |
| `nvme_subsystem_lookup_namespace` | `nvme_lookup_subsystem_namespace` |
| `nvme_subsystem_release_fds` | `nvme_release_subsystem_fds` |
| `nvme_get_path_attr` | — |
| `nvme_scan_namespace` | — |

---

## mi.h — NVMe-MI endpoint interface

| Function | Suggested Name |
|---|---|
| `nvme_mi_aem_aesi_get_aese` | `nvme_mi_get_aem_aesi_aese` |
| `nvme_mi_aem_aesi_get_aesid` | `nvme_mi_get_aem_aesi_aesid` |
| `nvme_mi_aem_aesi_set_aesid` | `nvme_mi_set_aem_aesi_aesid` |
| `nvme_mi_aem_aesi_set_aee` | `nvme_mi_set_aem_aesi_aee` |
| `nvme_mi_aem_aeei_get_aee` | `nvme_mi_get_aem_aeei_aee` |
| `nvme_mi_aem_aeei_get_aeeid` | `nvme_mi_get_aem_aeei_aeeid` |
| `nvme_mi_aem_aeei_set_aeeid` | `nvme_mi_set_aem_aeei_aeeid` |
| `nvme_mi_aem_aeei_set_aee` | `nvme_mi_set_aem_aeei_aee` |
| `nvme_mi_aem_aemti_get_aemgn` | `nvme_mi_get_aem_aemti_aemgn` |
| `nvme_mi_aem_aeolli_get_aeoltl` | `nvme_mi_get_aem_aeolli_aeoltl` |
| `nvme_mi_aem_aeolli_set_aeoltl` | `nvme_mi_set_aem_aeolli_aeoltl` |
| `nvme_mi_status_to_string` | — |
| `nvme_mi_set_csi` | — |
| `nvme_mi_first_endpoint` | — |
| `nvme_mi_next_endpoint` | — |
| `nvme_mi_for_each_endpoint` | — |
| `nvme_mi_for_each_endpoint_safe` | — |
| `nvme_mi_ep_set_timeout` | `nvme_mi_set_ep_timeout` |
| `nvme_mi_ep_set_mprt_max` | `nvme_mi_set_ep_mprt_max` |
| `nvme_mi_ep_get_timeout` | `nvme_mi_get_ep_timeout` |
| `nvme_mi_first_transport_handle` | — |
| `nvme_mi_next_transport_handle` | — |
| `nvme_mi_for_each_transport_handle` | — |
| `nvme_mi_for_each_transport_handle_safe` | — |
| `nvme_mi_open_mctp` | — |
| `nvme_mi_aem_open` | `nvme_mi_open_aem` |
| `nvme_mi_close` | — |
| `nvme_mi_scan_mctp` | — |
| `nvme_mi_scan_ep` | — |
| `nvme_mi_init_transport_handle` | — |
| `nvme_mi_ctrl_id` | `nvme_mi_get_ctrl_id` |
| `nvme_mi_endpoint_desc` | `nvme_mi_get_endpoint_desc` |
| `nvme_mi_mi_xfer` | — |
| `nvme_mi_mi_read_mi_data_subsys` | — |
| `nvme_mi_mi_read_mi_data_port` | — |
| `nvme_mi_mi_read_mi_data_ctrl_list` | — |
| `nvme_mi_mi_read_mi_data_ctrl` | — |
| `nvme_mi_mi_subsystem_health_status_poll` | `nvme_mi_mi_poll_subsystem_health_status` |
| `nvme_mi_mi_config_get` | `nvme_mi_mi_get_config` |
| `nvme_mi_mi_config_set` | `nvme_mi_mi_set_config` |
| `nvme_mi_mi_config_get_smbus_freq` | `nvme_mi_mi_get_config_smbus_freq` |
| `nvme_mi_mi_config_set_smbus_freq` | `nvme_mi_mi_set_config_smbus_freq` |
| `nvme_mi_mi_config_set_health_status_change` | `nvme_mi_mi_set_config_health_status_change` |
| `nvme_mi_mi_config_get_mctp_mtu` | `nvme_mi_mi_get_config_mctp_mtu` |
| `nvme_mi_mi_config_set_mctp_mtu` | `nvme_mi_mi_set_config_mctp_mtu` |
| `nvme_mi_mi_config_get_async_event` | `nvme_mi_mi_get_config_async_event` |
| `nvme_mi_mi_config_set_async_event` | `nvme_mi_mi_set_config_async_event` |
| `nvme_mi_admin_xfer` | — |
| `nvme_mi_control` | — |
| `nvme_mi_aem_get_next_event` | `nvme_mi_get_aem_next_event` |
| `nvme_mi_aem_get_fd` | `nvme_mi_get_aem_fd` |
| `nvme_mi_aem_enable` | `nvme_mi_enable_aem` |
| `nvme_mi_aem_get_enabled` | `nvme_mi_get_aem_enabled` |
| `nvme_mi_aem_disable` | `nvme_mi_disable_aem` |
| `nvme_mi_aem_process` | `nvme_mi_process_aem` |

---

## linux.h — Linux-specific key and identity utilities

| Function | Suggested Name |
|---|---|
| `nvme_gen_dhchap_key` | — |
| `nvme_lookup_keyring` | — |
| `nvme_describe_key_serial` | — |
| `nvme_lookup_key` | — |
| `nvme_set_keyring` | — |
| `nvme_read_key` | — |
| `nvme_update_key` | — |
| `nvme_scan_tls_keys` | — |
| `nvme_insert_tls_key` | — |
| `nvme_insert_tls_key_versioned` | — |
| `nvme_insert_tls_key_compat` | — |
| `nvme_generate_tls_key_identity` | — |
| `nvme_generate_tls_key_identity_compat` | — |
| `nvme_revoke_tls_key` | — |
| `nvme_export_tls_key` | — |
| `nvme_export_tls_key_versioned` | — |
| `nvme_import_tls_key` | — |
| `nvme_import_tls_key_versioned` | — |
| `nvme_hostnqn_generate` | `nvme_generate_hostnqn` |
| `nvme_hostnqn_generate_from_hostid` | `nvme_generate_hostnqn_from_hostid` |
| `nvme_hostid_generate` | `nvme_generate_hostid` |
| `nvme_hostnqn_from_file` | `nvme_read_hostnqn` |
| `nvme_hostid_from_file` | `nvme_read_hostid` |

---

## lib.h — libnvme library context and device management

| Function | Suggested Name |
|---|---|
| `nvme_create_global_ctx` | — |
| `nvme_free_global_ctx` | — |
| `nvme_set_logging_level` | — |
| `nvme_get_logging_level` | — |
| `nvme_open` | — |
| `nvme_close` | — |
| `nvme_transport_handle_get_fd` | `nvme_get_transport_handle_fd` |
| `nvme_transport_handle_get_name` | `nvme_get_transport_handle_name` |
| `nvme_transport_handle_is_blkdev` | `nvme_is_transport_handle_blkdev` |
| `nvme_transport_handle_is_chardev` | `nvme_is_transport_handle_chardev` |
| `nvme_transport_handle_is_direct` | `nvme_is_transport_handle_direct` |
| `nvme_transport_handle_is_mi` | `nvme_is_transport_handle_mi` |
| `nvme_transport_handle_set_submit_entry` | `nvme_set_transport_handle_submit_entry` |
| `nvme_transport_handle_set_submit_exit` | `nvme_set_transport_handle_submit_exit` |
| `nvme_transport_handle_set_decide_retry` | `nvme_set_transport_handle_decide_retry` |
| `nvme_set_probe_enabled` | — |
| `nvme_set_dry_run` | — |
| `nvme_set_ioctl_probing` | — |

---

## ioctl.h — NVMe IOCTL passthrough interface

| Function | Suggested Name |
|---|---|
| `nvme_submit_admin_passthru` | — |
| `nvme_submit_io_passthru` | — |
| `nvme_subsystem_reset` | `nvme_reset_subsystem` |
| `nvme_ctrl_reset` | `nvme_reset_ctrl` |
| `nvme_ns_rescan` | `nvme_rescan_ns` |
| `nvme_get_nsid` | — |

---

## filters.h — libnvme directory filter

| Function | Suggested Name |
|---|---|
| `nvme_namespace_filter` | `nvme_filter_namespace` |
| `nvme_paths_filter` | `nvme_filter_paths` |
| `nvme_ctrls_filter` | `nvme_filter_ctrls` |
| `nvme_subsys_filter` | `nvme_filter_subsys` |
| `nvme_scan_subsystems` | — |
| `nvme_scan_subsystem_namespaces` | — |
| `nvme_scan_ctrls` | — |
| `nvme_scan_ctrl_namespace_paths` | — |
| `nvme_scan_ctrl_namespaces` | — |
| `nvme_scan_ns_head_paths` | — |

---

## fabrics.h — NVMe-oF fabrics helper functions

| Function | Suggested Name |
|---|---|
| `nvme_parse_uri` | — |

---

## nbft.h — NVM Express Boot Firmware Table (NBFT) parser

| Function | Suggested Name |
|---|---|
| `nvme_nbft_read` | `nvme_read_nbft` |
| `nvme_nbft_free` | `nvme_free_nbft` |

---

## util.h — libnvme utility functions

| Function | Suggested Name |
|---|---|
| `nvme_status_to_errno` | — |
| `nvme_status_to_string` | — |
| `nvme_sanitize_ns_status_to_string` | — |
| `nvme_opcode_status_to_string` | — |
| `nvme_errno_to_string` | — |
| `nvme_strerror` | — |
| `nvme_get_version` | — |
| `nvme_uuid_to_string` | — |
| `nvme_uuid_from_string` | — |
| `nvme_uuid_random` | `nvme_random_uuid` |
| `nvme_uuid_find` | `nvme_find_uuid` |
| `nvme_basename` | — |
