// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libnvme.
 *
 * LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP (issue #3879, Phase 1): the
 * unprivileged side of the passthru channel. Marshals a real
 * struct libnvme_passthru_cmd into the wire protocol (privsep-proto.h),
 * sends it to the helper process on the other end of hdl->privsep_sock,
 * and unmarshals the response back into cmd.
 *
 * Every request this process sends is built fresh from @cmd; every
 * response is a complete, already-received copy in this process's own
 * memory before any of it is used. There is no shared/writable mapping
 * onto the helper's memory at any point.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libnvme.h>

#include <shared/compiler-attributes-util.h>

#include "private.h"
#include "privsep.h"
#include "privsep-proto.h"

static int privsep_passthru(struct libnvme_transport_handle *hdl,
		enum libnvme_privsep_op op, struct libnvme_passthru_cmd *cmd)
{
	/* struct libnvme_privsep_req/resp are several MB each
	 * (LIBNVME_PRIVSEP_MAX_XFER-sized inline buffers) -- heap-allocate,
	 * never put on the stack.
	 */
	struct libnvme_privsep_req *req;
	struct libnvme_privsep_resp *resp;
	int ret;

	if (cmd->data_len > LIBNVME_PRIVSEP_MAX_XFER)
		return -EMSGSIZE;

	req = malloc(sizeof(*req));
	resp = malloc(sizeof(*resp));
	if (!req || !resp) {
		ret = -ENOMEM;
		goto out;
	}

	req->op = op;
	req->opcode = cmd->opcode;
	req->flags = cmd->flags;
	req->nsid = cmd->nsid;
	req->cdw2 = cmd->cdw2;
	req->cdw3 = cmd->cdw3;
	req->cdw10 = cmd->cdw10;
	req->cdw11 = cmd->cdw11;
	req->cdw12 = cmd->cdw12;
	req->cdw13 = cmd->cdw13;
	req->cdw14 = cmd->cdw14;
	req->cdw15 = cmd->cdw15;
	req->timeout_ms = cmd->timeout_ms;
	req->data_len = cmd->data_len;

	if (cmd->data_len)
		memcpy(req->data, (void *)(uintptr_t)cmd->addr, cmd->data_len);

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: sending %s opcode=0x%02x nsid=0x%x cdw10=0x%x data_len=%u\n",
		    op == LIBNVME_PRIVSEP_OP_ADMIN ? "ADMIN" : "IO",
		    cmd->opcode, cmd->nsid, cmd->cdw10, cmd->data_len);

	if (libnvme_privsep_send_req(hdl->privsep_sock, req) !=
			(ssize_t)libnvme_privsep_req_len(req)) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to send request to helper\n");
		ret = -EIO;
		goto out;
	}

	ret = libnvme_privsep_recv_resp(hdl->privsep_sock, resp);
	if (ret <= 0) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to receive response from helper (%d)\n", ret);
		ret = ret == 0 ? -EPIPE : ret;
		goto out;
	}

	if (resp->data_len && resp->data_len == cmd->data_len)
		memcpy((void *)(uintptr_t)cmd->addr, resp->data, resp->data_len);

	cmd->result = resp->result;
	ret = resp->status;

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: received status=%d result=0x%" PRIx64 "\n",
		    ret, (uint64_t)cmd->result);

out:
	free(req);
	free(resp);

	return ret;
}

int __libnvme_privsep_admin_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd)
{
	return privsep_passthru(hdl, LIBNVME_PRIVSEP_OP_ADMIN, cmd);
}

int __libnvme_privsep_io_passthru(struct libnvme_transport_handle *hdl,
		struct libnvme_passthru_cmd *cmd)
{
	return privsep_passthru(hdl, LIBNVME_PRIVSEP_OP_IO, cmd);
}

int __libnvme_privsep_fabrics_connect(struct libnvme_transport_handle *hdl,
		const char *argstr, int *instance)
{
	struct libnvme_privsep_req *req;
	struct libnvme_privsep_resp *resp;
	size_t len = strlen(argstr) + 1;
	int ret;

	if (len > LIBNVME_PRIVSEP_MAX_XFER)
		return -EMSGSIZE;

	req = malloc(sizeof(*req));
	resp = malloc(sizeof(*resp));
	if (!req || !resp) {
		ret = -ENOMEM;
		goto out;
	}

	memset(req, 0, offsetof(struct libnvme_privsep_req, data));
	req->op = LIBNVME_PRIVSEP_OP_FABRICS_CONNECT;
	req->data_len = (uint32_t)len;
	memcpy(req->data, argstr, len);

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: sending FABRICS_CONNECT argstr_len=%zu\n", len);

	if (libnvme_privsep_send_req(hdl->privsep_sock, req) !=
			(ssize_t)libnvme_privsep_req_len(req)) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to send request to helper\n");
		ret = -EIO;
		goto out;
	}

	ret = libnvme_privsep_recv_resp(hdl->privsep_sock, resp);
	if (ret <= 0) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to receive response from helper (%d)\n", ret);
		ret = ret == 0 ? -EPIPE : ret;
		goto out;
	}

	if (resp->status == 0) {
		*instance = (int)resp->result;
		ret = *instance;
		libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
			    "privsep: received status=0 instance=%d\n", *instance);
	} else {
		ret = resp->status;
		libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
			    "privsep: received status=%d\n", resp->status);
	}

out:
	free(req);
	free(resp);

	return ret;
}

__shr_public int libnvme_privsep_open_device(struct libnvme_transport_handle *hdl,
		const char *devname, int flags)
{
	struct libnvme_privsep_req *req;
	struct libnvme_privsep_resp *resp;
	size_t len = strlen(devname) + 1;
	int ret;

	if (len > LIBNVME_PRIVSEP_MAX_XFER)
		return -EMSGSIZE;

	req = malloc(sizeof(*req));
	resp = malloc(sizeof(*resp));
	if (!req || !resp) {
		ret = -ENOMEM;
		goto out;
	}

	memset(req, 0, offsetof(struct libnvme_privsep_req, data));
	req->op = LIBNVME_PRIVSEP_OP_OPEN_DEVICE;
	req->cdw10 = (uint32_t)flags;
	req->data_len = (uint32_t)len;
	memcpy(req->data, devname, len);

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: sending OPEN_DEVICE devname=%s flags=0x%x\n",
		    devname, flags);

	if (libnvme_privsep_send_req(hdl->privsep_sock, req) !=
			(ssize_t)libnvme_privsep_req_len(req)) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to send request to helper\n");
		ret = -EIO;
		goto out;
	}

	ret = libnvme_privsep_recv_resp(hdl->privsep_sock, resp);
	if (ret <= 0) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to receive response from helper (%d)\n", ret);
		ret = ret == 0 ? -EPIPE : ret;
		goto out;
	}

	ret = resp->status;
	if (ret == 0)
		hdl->stat.st_mode = (mode_t)resp->result;

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: received status=%d\n", ret);

out:
	free(req);
	free(resp);

	return ret;
}

__shr_public int libnvme_privsep_raw_ioctl(struct libnvme_transport_handle *hdl,
		unsigned long request, void *arg, size_t arg_size)
{
	struct libnvme_privsep_req *req;
	struct libnvme_privsep_resp *resp;
	int ret;

	if (arg_size > LIBNVME_PRIVSEP_MAX_XFER)
		return -EMSGSIZE;

	req = malloc(sizeof(*req));
	resp = malloc(sizeof(*resp));
	if (!req || !resp) {
		ret = -ENOMEM;
		goto out;
	}

	memset(req, 0, offsetof(struct libnvme_privsep_req, data));
	req->op = LIBNVME_PRIVSEP_OP_RAW_IOCTL;
	req->cdw10 = (uint32_t)request;
	req->data_len = (uint32_t)arg_size;
	if (arg_size)
		memcpy(req->data, arg, arg_size);

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: sending RAW_IOCTL request=0x%lx arg_size=%zu\n",
		    request, arg_size);

	if (libnvme_privsep_send_req(hdl->privsep_sock, req) !=
			(ssize_t)libnvme_privsep_req_len(req)) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to send request to helper\n");
		ret = -EIO;
		goto out;
	}

	ret = libnvme_privsep_recv_resp(hdl->privsep_sock, resp);
	if (ret <= 0) {
		libnvme_msg(hdl->ctx, LIBNVME_LOG_WARN,
			    "privsep: failed to receive response from helper (%d)\n", ret);
		ret = ret == 0 ? -EPIPE : ret;
		goto out;
	}

	if (arg_size && resp->data_len == arg_size)
		memcpy(arg, resp->data, arg_size);

	ret = resp->status;

	libnvme_msg(hdl->ctx, LIBNVME_LOG_DEBUG,
		    "privsep: received status=%d\n", ret);

out:
	free(req);
	free(resp);

	return ret;
}

__shr_public int libnvme_open_privsep(struct libnvme_global_ctx *ctx, int sock,
		struct libnvme_transport_handle **hdlp)
{
	struct libnvme_transport_handle *hdl;

	hdl = __libnvme_create_transport_handle(ctx);
	if (!hdl)
		return -ENOMEM;

	hdl->type = LIBNVME_TRANSPORT_HANDLE_TYPE_PRIVSEP;
	hdl->uring_state = LIBNVME_IO_URING_STATE_NOT_AVAILABLE;
	hdl->privsep_sock = sock;

	*hdlp = hdl;

	return 0;
}

void __libnvme_privsep_close(struct libnvme_transport_handle *hdl)
{
	close(hdl->privsep_sock);
	free(hdl);
}
