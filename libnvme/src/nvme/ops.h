// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libnvme.
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors: Keith Busch <keith.busch@wdc.com>
 *	    Chaitanya Kulkarni <chaitanya.kulkarni@wdc.com>
 */

#pragma once

#include <stddef.h>

#include <nvme/cmds.h>

/**
 * DOC: ops.h
 *
 * NVMe command wrapper functions
 */

/**
 * nvme_fw_download_seq() - Firmware download sequence
 * @hdl:	Transport handle
 * @ish:	Ignore Shutdown (for NVMe-MI command)
 * @size:	Total size of the firmware image to transfer
 * @xfer:	Maximum size to send with each partial transfer
 * @offset:	Starting offset to send with this firmware download
 * @buf:	Address of buffer containing all or part of the firmware image.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_fw_download_seq(struct nvme_transport_handle *hdl, bool ish,
			__u32 size, __u32 xfer, __u32 offset, void *buf);

/**
 * nvme_set_etdas() - Set the Extended Telemetry Data Area 4 Supported bit
 * @hdl:	Transport handle
 * @changed:	boolean to indicate whether or not the host
 *		behavior support feature had been changed
 *
 * Return: The nvme command status if a response was received (see
 * &enum nvme_status_field) or -1 with errno set otherwise.
 */
int nvme_set_etdas(struct nvme_transport_handle *hdl, bool *changed);

/**
 * nvme_clear_etdas() - Clear the Extended Telemetry Data Area 4 Supported bit
 * @hdl:	Transport handle
 * @changed:	boolean to indicate whether or not the host
 *		behavior support feature had been changed
 *
 * Return: The nvme command status if a response was received (see
 * &enum nvme_status_field) or -1 with errno set otherwise.
 */
int nvme_clear_etdas(struct nvme_transport_handle *hdl, bool *changed);

/**
 * nvme_get_uuid_list - Returns the uuid list (if supported)
 * @hdl:	Transport handle
 * @uuid_list:	UUID list returned by identify UUID
 *
 * Return: The nvme command status if a response was received (see
 * &enum nvme_status_field) or -1 with errno set otherwise.
 */
int nvme_get_uuid_list(struct nvme_transport_handle *hdl,
		struct nvme_id_uuid_list *uuid_list);

/**
 * nvme_get_telemetry_max() - Get telemetry limits
 * @hdl:	Transport handle
 * @da:		On success return max supported data area
 * @max_data_tx: On success set to max transfer chunk supported by the controller
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_telemetry_max(struct nvme_transport_handle *hdl, enum nvme_telemetry_da *da, size_t *max_data_tx);

/**
 * nvme_get_telemetry_log() - Get specified telemetry log
 * @hdl:	Transport handle
 * @create:	Generate new host initated telemetry capture
 * @ctrl:	Get controller Initiated log
 * @rae:	Retain asynchronous events
 * @max_data_tx: Set the max data transfer size to be used retrieving telemetry.
 * @da:		Log page data area, valid values: &enum nvme_telemetry_da.
 * @log:	On success, set to the value of the allocated and retrieved log.
 * @size:	Ptr to the telemetry log size, so it can be returned
 *
 * The total size allocated can be calculated as:
 *   (nvme_telemetry_log da size  + 1) * NVME_LOG_TELEM_BLOCK_SIZE.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_telemetry_log(struct nvme_transport_handle *hdl, bool create, bool ctrl, bool rae, size_t max_data_tx,
			   enum nvme_telemetry_da da, struct nvme_telemetry_log **log,
			   size_t *size);
/**
 * nvme_get_ctrl_telemetry() - Get controller telemetry log
 * @hdl:	Transport handle
 * @rae:	Retain asynchronous events
 * @log:	On success, set to the value of the allocated and retrieved log.
 * @da:		Log page data area, valid values: &enum nvme_telemetry_da
 * @size:	Ptr to the telemetry log size, so it can be returned
 *
 * The total size allocated can be calculated as:
 *   (nvme_telemetry_log da size  + 1) * NVME_LOG_TELEM_BLOCK_SIZE.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_ctrl_telemetry(struct nvme_transport_handle *hdl, bool rae, struct nvme_telemetry_log **log,
		enum nvme_telemetry_da da, size_t *size);

/**
 * nvme_get_host_telemetry() - Get host telemetry log
 * @hdl:	Transport handle
 * @log:	On success, set to the value of the allocated and retrieved log.
 * @da:		Log page data area, valid values: &enum nvme_telemetry_da
 * @size:	Ptr to the telemetry log size, so it can be returned
 *
 * The total size allocated can be calculated as:
 *   (nvme_telemetry_log da size  + 1) * NVME_LOG_TELEM_BLOCK_SIZE.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_host_telemetry(struct nvme_transport_handle *hdl,  struct nvme_telemetry_log **log,
		enum nvme_telemetry_da da, size_t *size);

/**
 * nvme_get_new_host_telemetry() - Get new host telemetry log
 * @hdl:	Transport handle
 * @log:	On success, set to the value of the allocated and retrieved log.
 * @da:		Log page data area, valid values: &enum nvme_telemetry_da
 * @size:	Ptr to the telemetry log size, so it can be returned
 *
 * The total size allocated can be calculated as:
 *   (nvme_telemetry_log da size  + 1) * NVME_LOG_TELEM_BLOCK_SIZE.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_new_host_telemetry(struct nvme_transport_handle *hdl,  struct nvme_telemetry_log **log,
		enum nvme_telemetry_da da, size_t *size);

/**
 * nvme_get_ana_log_len_from_id_ctrl() - Retrieve maximum possible ANA log size
 * @id_ctrl:	Controller identify data
 * @rgo:	If true, return maximum log page size without NSIDs
 *
 * Return: A byte limit on the size of the controller's ANA log page
 */
size_t nvme_get_ana_log_len_from_id_ctrl(const struct nvme_id_ctrl *id_ctrl,
					 bool rgo);

/**
 * nvme_get_ana_log_len() - Retrieve size of the current ANA log
 * @hdl:	Transport handle
 * @analen:	Pointer to where the length will be set on success
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_ana_log_len(struct nvme_transport_handle *hdl, size_t *analen);

/**
 * nvme_get_logical_block_size() - Retrieve block size
 * @hdl:	Transport handle
 * @nsid:	Namespace id
 * @blksize:	Pointer to where the block size will be set on success
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_logical_block_size(struct nvme_transport_handle *hdl, __u32 nsid, int *blksize);

/**
 * nvme_get_lba_status_log() - Retrieve the LBA Status log page
 * @hdl:	Transport handle
 * @rae:	Retain asynchronous events
 * @log:	On success, set to the value of the allocated and retrieved log.
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_get_lba_status_log(struct nvme_transport_handle *hdl, bool rae, struct nvme_lba_status_log **log);

/**
 * nvme_namespace_attach_ctrls() - Attach namespace to controller(s)
 * @hdl:	Transport handle
 * @ish:	Ignore Shutdown (for NVMe-MI command)
 * @nsid:	Namespace ID to attach
 * @num_ctrls:	Number of controllers in ctrlist
 * @ctrlist:	List of controller IDs to perform the attach action
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_namespace_attach_ctrls(struct nvme_transport_handle *hdl, bool ish,
				__u32 nsid, __u16 num_ctrls, __u16 *ctrlist);

/**
 * nvme_namespace_detach_ctrls() - Detach namespace from controller(s)
 * @hdl:	Transport handle
 * @ish:	Ignore Shutdown (for NVMe-MI command)
 * @nsid:	Namespace ID to detach
 * @num_ctrls:	Number of controllers in ctrlist
 * @ctrlist:	List of controller IDs to perform the detach action
 *
 * Return: 0 on success, the nvme command status if a response was
 * received (see &enum nvme_status_field) or a negative error otherwise.
 */
int nvme_namespace_detach_ctrls(struct nvme_transport_handle *hdl, bool ish,
			__u32 nsid, __u16 num_ctrls, __u16 *ctrlist);

