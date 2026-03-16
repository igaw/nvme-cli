<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# libnvme API review

This document lists the currently exported function names. The goal is to make
the naming more consistent. Functions are grouped by common prefixes. Generic
top-level functions use the nvme_ prefix, while functions operating on specific
objects use the object type as a prefix (e.g., nvme_ns_). The prefix is
typically followed by a verb, such as nvme_create_ctrl.

---

## tree.h — libnvme tree object interface

| Function                             | Suggested Name           |
|--------------------------------------|--------------------------|
| `nvme_create_ctrl`                   |                          |
| `nvme_ctrl_config_match`             | `nvme_ctrl_match_config` |
| `nvme_ctrl_first_ns`                 |                          |
| `nvme_ctrl_first_path`               |                          |
| `nvme_ctrl_for_each_ns_safe`         |                          |
| `nvme_ctrl_for_each_ns`              |                          |
| `nvme_ctrl_for_each_path_safe`       |                          |
| `nvme_ctrl_for_each_path`            |                          |
| `nvme_ctrl_get_address`              |                          |
| `nvme_ctrl_get_config`               |                          |
| `nvme_ctrl_get_dhchap_host_key`      |                          |
| `nvme_ctrl_get_phy_slot`             |                          |
| `nvme_ctrl_get_src_addr`             |                          |
| `nvme_ctrl_get_state`                |                          |
| `nvme_ctrl_get_subsysnqn`            |                          |
| `nvme_ctrl_get_subsystem`            |                          |
| `nvme_ctrl_get_transport_handle`     |                          |
| `nvme_ctrl_identify`                 | `nvme_identify_ctrl`     |
| `nvme_ctrl_is_discovered`            |                          |
| `nvme_ctrl_is_discovery_ctrl`        |                          |
| `nvme_ctrl_is_persistent`            |                          |
| `nvme_ctrl_is_unique_discovery_ctrl` |                          |
| `nvme_ctrl_next_ns`                  |                          |
| `nvme_ctrl_next_path`                |                          |
| `nvme_ctrl_release_transport_handle` |                          |
| `nvme_ctrl_set_dhchap_host_key`      |                          |
| `nvme_disconnect_ctrl`               |                          |
| `nvme_dump_config`                   |                          |
| `nvme_dump_tree`                     |                          |
| `nvme_first_host`                    |                          |
| `nvme_first_subsystem`               |                          |
| `nvme_for_each_host_safe`            |                          |
| `nvme_for_each_host`                 |                          |
| `nvme_for_each_subsystem_safe`       |                          |
| `nvme_for_each_subsystem`            |                          |
| `nvme_free_ctrl`                     |                          |
| `nvme_free_host`                     |                          |
| `nvme_free_ns`                       |                          |
| `nvme_free_subsystem`                |                          |
| `nvme_get_application`               |                          |
| `nvme_get_attr`                      |                          |
| `nvme_get_ctrl_attr`                 |                          |
| `nvme_get_ns_attr`                   |                          |
| `nvme_get_path_attr`                 |                          |
| `nvme_get_subsys_attr`               |                          |
| `nvme_host_get_global_ctx`           |                          |
| `nvme_host_get_ids`                  |                          |
| `nvme_host_get`                      | `nvme_get_host`          |
| `nvme_host_is_pdc_enabled`           |                          |
| `nvme_host_release_fds`              |                          |
| `nvme_host_set_pdc_enabled`          |                          |
| `nvme_init_ctrl`                     |                          |
| `nvme_namespace_first_path`          |                          |
| `nvme_namespace_for_each_path_safe`  |                          |
| `nvme_namespace_for_each_path`       |                          |
| `nvme_namespace_next_path`           |                          |
| `nvme_next_host`                     |                          |
| `nvme_next_subsystem`                |                          |
| `nvme_ns_compare`                    |                          |
| `nvme_ns_flush`                      |                          |
| `nvme_ns_get_csi`                    |                          |
| `nvme_ns_get_ctrl`                   |                          |
| `nvme_ns_get_eui64`                  |                          |
| `nvme_ns_get_firmware`               |                          |
| `nvme_ns_get_generic_name`           |                          |
| `nvme_ns_get_model`                  |                          |
| `nvme_ns_get_nguid`                  |                          |
| `nvme_ns_get_serial`                 |                          |
| `nvme_ns_get_subsystem`              |                          |
| `nvme_ns_get_uuid`                   |                          |
| `nvme_ns_head_get_sysfs_dir`         |                          |
| `nvme_ns_identify_descs`             |                          |
| `nvme_ns_identify`                   |                          |
| `nvme_ns_read`                       |                          |
| `nvme_ns_verify`                     |                          |
| `nvme_ns_write_uncorrectable`        |                          |
| `nvme_ns_write_zeros`                |                          |
| `nvme_ns_write`                      |                          |
| `nvme_path_get_ctrl`                 |                          |
| `nvme_path_get_ns`                   |                          |
| `nvme_path_get_queue_depth`          |                          |
| `nvme_read_config`                   |                          |
| `nvme_refresh_topology`              |                          |
| `nvme_release_fds`                   |                          |
| `nvme_rescan_ctrl`                   |                          |
| `nvme_scan_ctrl`                     |                          |
| `nvme_scan_namespace`                |                          |
| `nvme_scan_topology`                 |                          |
| `nvme_set_application`               |                          |
| `nvme_skip_namespaces`               |                          |
| `nvme_subsystem_first_ctrl`          |                          |
| `nvme_subsystem_first_ns`            |                          |
| `nvme_subsystem_for_each_ctrl_safe`  |                          |
| `nvme_subsystem_for_each_ctrl`       |                          |
| `nvme_subsystem_for_each_ns_safe`    |
| `nvme_subsystem_for_each_ns`         |                          |
| `nvme_subsystem_get_fw_rev`          |                          |
| `nvme_subsystem_get_host`            |                          |
| `nvme_subsystem_get_nqn`             |                          |
| `nvme_subsystem_get_type`            |                          |
| `nvme_subsystem_get`                 | `nvme_get_subsystem`     |
| `nvme_subsystem_lookup_namespace`    |                          |
| `nvme_subsystem_next_ctrl`           |                          |
| `nvme_subsystem_next_ns`             |                          |
| `nvme_subsystem_release_fds`         |                          |
| `nvme_unlink_ctrl`                   |                          |
---

## linux.h — Linux-specific key and identity utilities

| Function                                | Suggested Name                      |
|-----------------------------------------|-------------------------------------|
| `nvme_describe_key_serial`              |                                     |
| `nvme_export_tls_key_versioned`         |                                     |
| `nvme_export_tls_key`                   |                                     |
| `nvme_gen_dhchap_key`                   |                                     |
| `nvme_generate_tls_key_identity_compat` |                                     |
| `nvme_generate_tls_key_identity`        |                                     |
| `nvme_hostid_from_file`                 | `nvme_read_hostid`                  |
| `nvme_hostid_generate`                  | `nvme_generate_hostid`              |
| `nvme_hostnqn_from_file`                | `nvme_read_hostnqn`                 |
| `nvme_hostnqn_generate_from_hostid`     | `nvme_generate_hostnqn_from_hostid` |
| `nvme_hostnqn_generate`                 | `nvme_generate_hostnqn`             |
| `nvme_import_tls_key_versioned`         |                                     |
| `nvme_import_tls_key`                   |                                     |
| `nvme_insert_tls_key_compat`            |                                     |
| `nvme_insert_tls_key_versioned`         |                                     |
| `nvme_insert_tls_key`                   |                                     |
| `nvme_lookup_key`                       |                                     |
| `nvme_lookup_keyring`                   |                                     |
| `nvme_read_key`                         |                                     |
| `nvme_revoke_tls_key`                   |                                     |
| `nvme_scan_tls_keys`                    |                                     |
| `nvme_set_keyring`                      |                                     |
| `nvme_update_key`                       |                                     |

---

## lib.h — libnvme library context and device management

| Function                                 | Suggested Name |
|------------------------------------------|----------------|
| `nvme_close`                             |                |
| `nvme_create_global_ctx`                 |                |
| `nvme_free_global_ctx`                   |                |
| `nvme_get_logging_level`                 |                |
| `nvme_open`                              |                |
| `nvme_set_dry_run`                       |                |
| `nvme_set_ioctl_probing`                 |                |
| `nvme_set_logging_level`                 |                |
| `nvme_set_probe_enabled`                 |                |
| `nvme_transport_handle_get_fd`           |                |
| `nvme_transport_handle_get_name`         |                |
| `nvme_transport_handle_is_blkdev`        |                |
| `nvme_transport_handle_is_chardev`       |                |
| `nvme_transport_handle_is_direct`        |                |
| `nvme_transport_handle_is_mi`            |                |
| `nvme_transport_handle_set_decide_retry` |                |
| `nvme_transport_handle_set_submit_entry` |                |
| `nvme_transport_handle_set_submit_exit`  |                |

---

## ioctl.h — NVMe IOCTL passthrough interface

| Function                     | Suggested Name         |
|------------------------------|------------------------|
| `nvme_ctrl_reset`            | `nvme_reset_ctrl`      |
| `nvme_get_nsid`              |                        |
| `nvme_ns_rescan`             | `nvme_rescan_ns`       |
| `nvme_submit_admin_passthru` |                        |
| `nvme_submit_io_passthru`    |                        |
| `nvme_subsystem_reset`       | `nvme_reset_subsystem` |

---

## filters.h — libnvme directory filter

| Function                         | Suggested Name          |
|----------------------------------|-------------------------|
| `nvme_ctrls_filter`              | `nvme_filter_ctrls`     |
| `nvme_namespace_filter`          | `nvme_filter_namespace` |
| `nvme_paths_filter`              | `nvme_filter_paths`     |
| `nvme_scan_ctrl_namespace_paths` |                         |
| `nvme_scan_ctrl_namespaces`      |                         |
| `nvme_scan_ctrls`                |                         |
| `nvme_scan_ns_head_paths`        |                         |
| `nvme_scan_subsystem_namespaces` |                         |
| `nvme_scan_subsystems`           |                         |
| `nvme_subsys_filter`             | `nvme_filter_subsys`    |

---

## fabrics.h — NVMe-oF fabrics helper functions

| Function         | Suggested Name |
|------------------|----------------|
| `nvme_parse_uri` |                |

---

## nbft.h — NVM Express Boot Firmware Table (NBFT) parser

| Function         | Suggested Name   |
|------------------|------------------|
| `nvme_nbft_free` | `nvme_free_nbft` |
| `nvme_nbft_read` | `nvme_read_nbft` |

---

## util.h — libnvme utility functions

| Function                            | Suggested Name     |
|-------------------------------------|--------------------|
| `nvme_basename`                     |                    |
| `nvme_errno_to_string`              |                    |
| `nvme_get_version`                  |                    |
| `nvme_opcode_status_to_string`      |                    |
| `nvme_sanitize_ns_status_to_string` |                    |
| `nvme_status_to_errno`              |                    |
| `nvme_status_to_string`             |                    |
| `nvme_strerror`                     |                    |
| `nvme_uuid_find`                    | `nvme_find_uuid`   |
| `nvme_uuid_from_string`             |                    |
| `nvme_uuid_random`                  | `nvme_random_uuid` |
| `nvme_uuid_to_string`               |                    |
