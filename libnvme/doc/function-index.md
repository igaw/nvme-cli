<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# libnvme API Function Index

Quick-reference table of all public functions exported by libnvme, grouped by
header file.

---

## tree.h — libnvme tree object interface

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_set_application` | Specify managing application | — |
| `nvme_get_application` | Get managing application | — |
| `nvme_skip_namespaces` | Skip namespace scanning | — |
| `nvme_release_fds` | Close all opened file descriptors in the tree | — |
| `nvme_first_host` | Start host iterator | — |
| `nvme_next_host` | Next host iterator | — |
| `nvme_host_get_global_ctx` | Returns nvme_global_ctx object | `nvme_get_host_global_ctx` |
| `nvme_host_set_pdc_enabled` | Set Persistent Discovery Controller flag | `nvme_set_host_pdc_enabled` |
| `nvme_host_is_pdc_enabled` | Is Persistent Discovery Controller enabled | `nvme_is_host_pdc_enabled` |
| `nvme_host_get` | Returns a host object | `nvme_get_host` |
| `nvme_host_get_ids` | Retrieve host ids from various sources | `nvme_get_host_ids` |
| `nvme_first_subsystem` | Start subsystem iterator | — |
| `nvme_next_subsystem` | Next subsystem iterator | — |
| `nvme_subsystem_get` | Returns nvme_subsystem_t object | `nvme_get_subsystem` |
| `nvme_free_subsystem` | Free a subsystem | — |
| `nvme_subsystem_get_host` | Returns nvme_host_t object | `nvme_get_subsystem_host` |
| `nvme_ctrl_first_ns` | Start namespace iterator | `nvme_first_ctrl_ns` |
| `nvme_ctrl_next_ns` | Next namespace iterator | `nvme_next_ctrl_ns` |
| `nvme_ctrl_first_path` | Start path iterator | `nvme_first_ctrl_path` |
| `nvme_ctrl_next_path` | Next path iterator | `nvme_next_ctrl_path` |
| `nvme_subsystem_first_ctrl` | First ctrl iterator | `nvme_first_subsystem_ctrl` |
| `nvme_subsystem_next_ctrl` | Next ctrl iterator | `nvme_next_subsystem_ctrl` |
| `nvme_namespace_first_path` | Start path iterator | `nvme_first_namespace_path` |
| `nvme_namespace_next_path` | Next path iterator | `nvme_next_namespace_path` |
| `nvme_ctrl_config_match` | Check if ctrl matches config params | `nvme_match_ctrl_config` |
| `nvme_create_ctrl` | Allocate an unconnected NVMe controller | — |
| `nvme_subsystem_first_ns` | Start namespace iterator | `nvme_first_subsystem_ns` |
| `nvme_subsystem_next_ns` | Next namespace iterator | `nvme_next_subsystem_ns` |
| `nvme_for_each_host_safe` | Traverse host list (safe) | — |
| `nvme_for_each_host` | Traverse host list | — |
| `nvme_for_each_subsystem_safe` | Traverse subsystems (safe) | — |
| `nvme_for_each_subsystem` | Traverse subsystems | — |
| `nvme_subsystem_for_each_ctrl_safe` | Traverse controllers (safe) | `nvme_for_each_subsystem_ctrl_safe` |
| `nvme_subsystem_for_each_ctrl` | Traverse controllers | `nvme_for_each_subsystem_ctrl` |
| `nvme_ctrl_for_each_ns_safe` | Traverse namespaces (safe) | `nvme_for_each_ctrl_ns_safe` |
| `nvme_ctrl_for_each_ns` | Traverse namespaces | `nvme_for_each_ctrl_ns` |
| `nvme_ctrl_for_each_path_safe` | Traverse paths (safe) | `nvme_for_each_ctrl_path_safe` |
| `nvme_ctrl_for_each_path` | Traverse paths | `nvme_for_each_ctrl_path` |
| `nvme_subsystem_for_each_ns_safe` | Traverse namespaces under subsystem (safe) | `nvme_for_each_subsystem_ns_safe` |
| `nvme_subsystem_for_each_ns` | Traverse namespaces under subsystem | `nvme_for_each_subsystem_ns` |
| `nvme_namespace_for_each_path_safe` | Traverse paths under namespace (safe) | `nvme_for_each_namespace_path_safe` |
| `nvme_namespace_for_each_path` | Traverse paths under namespace | `nvme_for_each_namespace_path` |
| `nvme_ns_get_csi` | Command set identifier of a namespace | `nvme_get_ns_csi` |
| `nvme_ns_get_eui64` | 64-bit eui of a namespace | `nvme_get_ns_eui64` |
| `nvme_ns_get_nguid` | 128-bit nguid of a namespace | `nvme_get_ns_nguid` |
| `nvme_ns_get_uuid` | UUID of a namespace | `nvme_get_ns_uuid` |
| `nvme_ns_get_generic_name` | Returns name of generic namespace chardev | `nvme_get_ns_generic_name` |
| `nvme_ns_get_firmware` | Firmware string of a namespace | `nvme_get_ns_firmware` |
| `nvme_ns_get_serial` | Serial number of a namespace | `nvme_get_ns_serial` |
| `nvme_ns_get_model` | Model of a namespace | `nvme_get_ns_model` |
| `nvme_ns_get_subsystem` | nvme_subsystem_t of a namespace | `nvme_get_ns_subsystem` |
| `nvme_ns_get_ctrl` | nvme_ctrl_t of a namespace | `nvme_get_ns_ctrl` |
| `nvme_free_ns` | Free a namespace object | — |
| `nvme_ns_read` | Read from a namespace | `nvme_read_ns` |
| `nvme_ns_write` | Write to a namespace | `nvme_write_ns` |
| `nvme_ns_verify` | Verify data on a namespace | `nvme_verify_ns` |
| `nvme_ns_compare` | Compare data on a namespace | `nvme_compare_ns` |
| `nvme_ns_write_zeros` | Write zeros to a namespace | `nvme_write_ns_zeros` |
| `nvme_ns_write_uncorrectable` | Issue a 'write uncorrectable' command | `nvme_write_ns_uncorrectable` |
| `nvme_ns_flush` | Flush data to a namespace | `nvme_flush_ns` |
| `nvme_ns_identify` | Issue an 'identify namespace' command | `nvme_identify_ns` |
| `nvme_ns_identify_descs` | Issue an 'identify descriptors' command | `nvme_identify_ns_descs` |
| `nvme_path_get_queue_depth` | Queue depth of an nvme_path_t object | `nvme_get_path_queue_depth` |
| `nvme_path_get_ctrl` | Parent controller of an nvme_path_t object | `nvme_get_path_ctrl` |
| `nvme_path_get_ns` | Parent namespace of an nvme_path_t object | `nvme_get_path_ns` |
| `nvme_ctrl_get_transport_handle` | Get associated transport handle | `nvme_get_ctrl_transport_handle` |
| `nvme_ctrl_release_transport_handle` | Free transport handle from controller object | `nvme_release_ctrl_transport_handle` |
| `nvme_ctrl_get_address` | Address string of a controller | `nvme_get_ctrl_address` |
| `nvme_ctrl_get_src_addr` | Extract src_addr from the address string | `nvme_get_ctrl_src_addr` |
| `nvme_ctrl_get_phy_slot` | PCI physical slot number of a controller | `nvme_get_ctrl_phy_slot` |
| `nvme_ctrl_get_state` | Running state of a controller | `nvme_get_ctrl_state` |
| `nvme_ctrl_get_subsysnqn` | Subsystem NQN of a controller | `nvme_get_ctrl_subsysnqn` |
| `nvme_ctrl_get_subsystem` | Parent subsystem of a controller | `nvme_get_ctrl_subsystem` |
| `nvme_ctrl_get_dhchap_host_key` | Return host key | `nvme_get_ctrl_dhchap_host_key` |
| `nvme_ctrl_set_dhchap_host_key` | Set host key | `nvme_set_ctrl_dhchap_host_key` |
| `nvme_ns_head_get_sysfs_dir` | sysfs dir of namespace head | `nvme_get_ns_head_sysfs_dir` |
| `nvme_ctrl_get_config` | Fabrics configuration of a controller | `nvme_get_ctrl_config` |
| `nvme_ctrl_is_discovered` | Returns the value of the 'discovered' flag | `nvme_is_ctrl_discovered` |
| `nvme_ctrl_is_persistent` | Returns the value of the 'persistent' flag | `nvme_is_ctrl_persistent` |
| `nvme_ctrl_is_discovery_ctrl` | Check the 'discovery_ctrl' flag | `nvme_is_ctrl_discovery_ctrl` |
| `nvme_ctrl_is_unique_discovery_ctrl` | Check the 'unique_discovery_ctrl' flag | `nvme_is_ctrl_unique_discovery_ctrl` |
| `nvme_ctrl_identify` | Issue an 'identify controller' command | `nvme_identify_ctrl` |
| `nvme_disconnect_ctrl` | Disconnect a controller | — |
| `nvme_scan_ctrl` | Scan on a controller | — |
| `nvme_rescan_ctrl` | Rescan an existing controller | — |
| `nvme_init_ctrl` | Initialize nvme_ctrl_t object for an existing controller | — |
| `nvme_free_ctrl` | Free controller | — |
| `nvme_unlink_ctrl` | Unlink controller | — |
| `nvme_subsystem_get_nqn` | Retrieve NQN from subsystem | `nvme_get_subsystem_nqn` |
| `nvme_subsystem_get_type` | Returns the type of a subsystem | `nvme_get_subsystem_type` |
| `nvme_subsystem_get_fw_rev` | Return the firmware rev of subsystem | `nvme_get_subsystem_fw_rev` |
| `nvme_scan_topology` | Scan NVMe topology and apply filter | — |
| `nvme_host_release_fds` | Close all opened file descriptors under host | `nvme_release_host_fds` |
| `nvme_free_host` | Free nvme_host_t object | — |
| `nvme_read_config` | Read NVMe JSON configuration file | — |
| `nvme_refresh_topology` | Refresh nvme_root_t object contents | — |
| `nvme_dump_config` | Print the JSON configuration | — |
| `nvme_dump_tree` | Dump internal object tree | — |
| `nvme_get_attr` | Read sysfs attribute | — |
| `nvme_get_subsys_attr` | Read subsystem sysfs attribute | — |
| `nvme_get_ctrl_attr` | Read controller sysfs attribute | — |
| `nvme_get_ns_attr` | Read namespace sysfs attribute | — |
| `nvme_subsystem_lookup_namespace` | Lookup namespace by NSID | `nvme_lookup_subsystem_namespace` |
| `nvme_subsystem_release_fds` | Close all opened fds under subsystem | `nvme_release_subsystem_fds` |
| `nvme_get_path_attr` | Read path sysfs attribute | — |
| `nvme_scan_namespace` | Scan namespace based on sysfs name | — |

---

## mi.h — NVMe-MI endpoint interface

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_mi_aem_aesi_get_aese` | Return aese from aesi field | `nvme_mi_get_aem_aesi_aese` |
| `nvme_mi_aem_aesi_get_aesid` | Return aesid from aesi field | `nvme_mi_get_aem_aesi_aesid` |
| `nvme_mi_aem_aesi_set_aesid` | Set aesid in the aesi field | `nvme_mi_set_aem_aesi_aesid` |
| `nvme_mi_aem_aesi_set_aee` | Set aee in the aesi field | `nvme_mi_set_aem_aesi_aee` |
| `nvme_mi_aem_aeei_get_aee` | Return aee from aeei field | `nvme_mi_get_aem_aeei_aee` |
| `nvme_mi_aem_aeei_get_aeeid` | Return aeeid from aeei field | `nvme_mi_get_aem_aeei_aeeid` |
| `nvme_mi_aem_aeei_set_aeeid` | Set aeeid in the aeei field | `nvme_mi_set_aem_aeei_aeeid` |
| `nvme_mi_aem_aeei_set_aee` | Set aee in the aeei field | `nvme_mi_set_aem_aeei_aee` |
| `nvme_mi_aem_aemti_get_aemgn` | Return aemgn from aemti field | `nvme_mi_get_aem_aemti_aemgn` |
| `nvme_mi_aem_aeolli_get_aeoltl` | Return aeoltl from aeolli field | `nvme_mi_get_aem_aeolli_aeoltl` |
| `nvme_mi_aem_aeolli_set_aeoltl` | Set aeoltl in the aeolli field | `nvme_mi_set_aem_aeolli_aeoltl` |
| `nvme_mi_status_to_string` | Return a string representation of the MI status | — |
| `nvme_mi_set_csi` | Assign a CSI to an endpoint | — |
| `nvme_mi_first_endpoint` | Start endpoint iterator | — |
| `nvme_mi_next_endpoint` | Continue endpoint iterator | — |
| `nvme_mi_for_each_endpoint` | Iterator for NVMe-MI endpoints | — |
| `nvme_mi_for_each_endpoint_safe` | Iterator for NVMe-MI endpoints, allowing removal | — |
| `nvme_mi_ep_set_timeout` | Set a timeout for NVMe-MI responses | `nvme_mi_set_ep_timeout` |
| `nvme_mi_ep_set_mprt_max` | Set the maximum wait time for a More Processing Required response | `nvme_mi_set_ep_mprt_max` |
| `nvme_mi_ep_get_timeout` | Get the current timeout value for NVMe-MI responses | `nvme_mi_get_ep_timeout` |
| `nvme_mi_first_transport_handle` | Start transport handle iterator | — |
| `nvme_mi_next_transport_handle` | Continue transport handle iterator | — |
| `nvme_mi_for_each_transport_handle` | Iterator for transport handles to NVMe-MI controllers | — |
| `nvme_mi_for_each_transport_handle_safe` | Iterator for transport handles, allowing removal | — |
| `nvme_mi_open_mctp` | Create an endpoint using a MCTP connection | — |
| `nvme_mi_aem_open` | Prepare an existing endpoint to receive AEMs | `nvme_mi_open_aem` |
| `nvme_mi_close` | Close an endpoint connection and release resources | — |
| `nvme_mi_scan_mctp` | Look for MCTP-connected NVMe-MI endpoints | — |
| `nvme_mi_scan_ep` | Query an endpoint for its NVMe controllers | — |
| `nvme_mi_init_transport_handle` | Initialise a transport handle to an NVMe controller | — |
| `nvme_mi_ctrl_id` | Get the ID of a controller | `nvme_mi_get_ctrl_id` |
| `nvme_mi_endpoint_desc` | Get a string describing a MI endpoint | `nvme_mi_get_endpoint_desc` |
| `nvme_mi_mi_xfer` | Raw MI transfer interface | — |
| `nvme_mi_mi_read_mi_data_subsys` | Perform a Read MI Data Structure command (subsystem) | — |
| `nvme_mi_mi_read_mi_data_port` | Perform a Read MI Data Structure command (port) | — |
| `nvme_mi_mi_read_mi_data_ctrl_list` | Perform a Read MI Data Structure command (controller list) | — |
| `nvme_mi_mi_read_mi_data_ctrl` | Perform a Read MI Data Structure command (controller) | — |
| `nvme_mi_mi_subsystem_health_status_poll` | Read the Subsystem Health Data Structure | `nvme_mi_mi_poll_subsystem_health_status` |
| `nvme_mi_mi_config_get` | Query a configuration parameter | `nvme_mi_mi_get_config` |
| `nvme_mi_mi_config_set` | Set a configuration parameter | `nvme_mi_mi_set_config` |
| `nvme_mi_mi_config_get_smbus_freq` | Get configuration: SMBus port frequency | `nvme_mi_mi_get_config_smbus_freq` |
| `nvme_mi_mi_config_set_smbus_freq` | Set configuration: SMBus port frequency | `nvme_mi_mi_set_config_smbus_freq` |
| `nvme_mi_mi_config_set_health_status_change` | Clear CCS bits in health status | `nvme_mi_mi_set_config_health_status_change` |
| `nvme_mi_mi_config_get_mctp_mtu` | Get configuration: MCTP MTU | `nvme_mi_mi_get_config_mctp_mtu` |
| `nvme_mi_mi_config_set_mctp_mtu` | Set configuration: MCTP MTU | `nvme_mi_mi_set_config_mctp_mtu` |
| `nvme_mi_mi_config_get_async_event` | Get configuration: Asynchronous Event | `nvme_mi_mi_get_config_async_event` |
| `nvme_mi_mi_config_set_async_event` | Set configuration: Asynchronous Event | `nvme_mi_mi_set_config_async_event` |
| `nvme_mi_admin_xfer` | Raw admin transfer interface | — |
| `nvme_mi_control` | Perform a Control Primitive command | — |
| `nvme_mi_aem_get_next_event` | Get details for the next event to parse | `nvme_mi_get_aem_next_event` |
| `nvme_mi_aem_get_fd` | Returns the pollable fd for AEM data available | `nvme_mi_get_aem_fd` |
| `nvme_mi_aem_enable` | Enable AE on the provided endpoint | `nvme_mi_enable_aem` |
| `nvme_mi_aem_get_enabled` | Return information on which AEs are enabled | `nvme_mi_get_aem_enabled` |
| `nvme_mi_aem_disable` | Disable AE on the provided endpoint | `nvme_mi_disable_aem` |
| `nvme_mi_aem_process` | Process AEM on the provided endpoint | `nvme_mi_process_aem` |

---

## linux.h — Linux-specific key and identity utilities

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_gen_dhchap_key` | DH-HMAC-CHAP key generation | — |
| `nvme_lookup_keyring` | Lookup keyring serial number | — |
| `nvme_describe_key_serial` | Return key description | — |
| `nvme_lookup_key` | Lookup key serial number | — |
| `nvme_set_keyring` | Link keyring for lookup | — |
| `nvme_read_key` | Read key raw data | — |
| `nvme_update_key` | Update key raw data | — |
| `nvme_scan_tls_keys` | Iterate over TLS keys in a keyring | — |
| `nvme_insert_tls_key` | Derive and insert TLS key | — |
| `nvme_insert_tls_key_versioned` | Derive and insert TLS key (versioned) | — |
| `nvme_insert_tls_key_compat` | Derive and insert TLS key (compat) | — |
| `nvme_generate_tls_key_identity` | Generate the TLS key identity | — |
| `nvme_generate_tls_key_identity_compat` | Generate the TLS key identity (compat) | — |
| `nvme_revoke_tls_key` | Revoke TLS key from keyring | — |
| `nvme_export_tls_key` | Export a TLS key | — |
| `nvme_export_tls_key_versioned` | Export a TLS pre-shared key (versioned) | — |
| `nvme_import_tls_key` | Import a TLS key | — |
| `nvme_import_tls_key_versioned` | Import a TLS key (versioned) | — |
| `nvme_hostnqn_generate` | Generate a machine specific host NQN | `nvme_generate_hostnqn` |
| `nvme_hostnqn_generate_from_hostid` | Generate a host NQN from a host identifier | `nvme_generate_hostnqn_from_hostid` |
| `nvme_hostid_generate` | Generate a machine specific host identifier | `nvme_generate_hostid` |
| `nvme_hostnqn_from_file` | Read the host NQN from the config file | `nvme_read_hostnqn` |
| `nvme_hostid_from_file` | Read the host identifier from the config file | `nvme_read_hostid` |

---

## lib.h — libnvme library context and device management

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_create_global_ctx` | Initialize global context object | — |
| `nvme_free_global_ctx` | Free global context object | — |
| `nvme_set_logging_level` | Set current logging level | — |
| `nvme_get_logging_level` | Get current logging level | — |
| `nvme_open` | Open an nvme controller or namespace device | — |
| `nvme_close` | Close transport handle | — |
| `nvme_transport_handle_get_fd` | Return file descriptor from transport handle | `nvme_get_transport_handle_fd` |
| `nvme_transport_handle_get_name` | Return name of the device transport handle | `nvme_get_transport_handle_name` |
| `nvme_transport_handle_is_blkdev` | Check if transport handle is a block device | `nvme_is_transport_handle_blkdev` |
| `nvme_transport_handle_is_chardev` | Check if transport handle is a char device | `nvme_is_transport_handle_chardev` |
| `nvme_transport_handle_is_direct` | Check if transport handle is using IOCTL | `nvme_is_transport_handle_direct` |
| `nvme_transport_handle_is_mi` | Check if transport handle is using MI | `nvme_is_transport_handle_mi` |
| `nvme_transport_handle_set_submit_entry` | Install a submit-entry callback | `nvme_set_transport_handle_submit_entry` |
| `nvme_transport_handle_set_submit_exit` | Install a submit-exit callback | `nvme_set_transport_handle_submit_exit` |
| `nvme_transport_handle_set_decide_retry` | Install a retry-decision callback | `nvme_set_transport_handle_decide_retry` |
| `nvme_set_probe_enabled` | Enable/disable the probe for new MI endpoints | — |
| `nvme_set_dry_run` | Set global dry run state | — |
| `nvme_set_ioctl_probing` | Enable/disable 64-bit IOCTL probing | — |

---

## ioctl.h — NVMe IOCTL passthrough interface

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_submit_admin_passthru` | Submit an nvme passthrough admin command | — |
| `nvme_submit_io_passthru` | Submit an nvme passthrough I/O command | — |
| `nvme_subsystem_reset` | Initiate a subsystem reset | `nvme_reset_subsystem` |
| `nvme_ctrl_reset` | Initiate a controller reset | `nvme_reset_ctrl` |
| `nvme_ns_rescan` | Initiate a controller rescan | `nvme_rescan_ns` |
| `nvme_get_nsid` | Retrieve the NSID from a namespace file descriptor | — |

---

## filters.h — libnvme directory filter

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_namespace_filter` | Filter for namespaces | `nvme_filter_namespace` |
| `nvme_paths_filter` | Filter for paths | `nvme_filter_paths` |
| `nvme_ctrls_filter` | Filter for controllers | `nvme_filter_ctrls` |
| `nvme_subsys_filter` | Filter for subsystems | `nvme_filter_subsys` |
| `nvme_scan_subsystems` | Scan for subsystems | — |
| `nvme_scan_subsystem_namespaces` | Scan for namespaces in a subsystem | — |
| `nvme_scan_ctrls` | Scan for controllers | — |
| `nvme_scan_ctrl_namespace_paths` | Scan for namespace paths in a controller | — |
| `nvme_scan_ctrl_namespaces` | Scan for namespaces in a controller | — |
| `nvme_scan_ns_head_paths` | Scan for namespace paths | — |

---

## fabrics.h — NVMe-oF fabrics helper functions

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_parse_uri` | Parse the URI string | — |

---

## nbft.h — NVM Express Boot Firmware Table (NBFT) parser

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_nbft_read` | Read and parse contents of an ACPI NBFT table | `nvme_read_nbft` |
| `nvme_nbft_free` | Free the struct nbft_info and its contents | `nvme_free_nbft` |

---

## util.h — libnvme utility functions

| Function | Description | Suggested Name |
|---|---|---|
| `nvme_status_to_errno` | Convert nvme return status to errno | — |
| `nvme_status_to_string` | Return string describing nvme return status | — |
| `nvme_sanitize_ns_status_to_string` | Return sanitize ns status string | — |
| `nvme_opcode_status_to_string` | Return nvme opcode status string | — |
| `nvme_errno_to_string` | Return string describing nvme connect failures | — |
| `nvme_strerror` | Return string describing nvme errors and errno | — |
| `nvme_get_version` | Return libnvme version string | — |
| `nvme_uuid_to_string` | Return string representation of encoded UUID | — |
| `nvme_uuid_from_string` | Return encoded UUID representation of string UUID | — |
| `nvme_uuid_random` | Generate random UUID | `nvme_random_uuid` |
| `nvme_uuid_find` | Find UUID position on UUID list | `nvme_find_uuid` |
| `nvme_basename` | Return the final path component (after the last '/') | — |
