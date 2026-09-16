// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * The "privileged" side of the Phase-0 privsep spike (issue #3879).
 *
 * Deliberately minimal: every request is a complete, already-received copy
 * in this process's own memory (proto.h's privsep_recv_req() guarantees
 * that) before any of it is used. There is no shared/writable mapping onto
 * the other side's memory, and no allowlist/seccomp/capability-drop here --
 * that hardening is Phase 4, once this shape has been validated.
 */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libnvme.h>

#include "nvme/loopback.h"
#include "proto.h"

void child_main(int sock, struct libnvme_transport_handle *hdl)
{
	struct privsep_req *req = malloc(sizeof(*req));
	struct privsep_resp *resp = malloc(sizeof(*resp));

	if (!req || !resp)
		_exit(1);

	for (;;) {
		struct libnvme_passthru_cmd cmd = {};
		int ret = privsep_recv_req(sock, req);

		if (ret == 0)
			break; /* parent closed the socket: normal shutdown */

		memset(resp, 0, offsetof(struct privsep_resp, data));

		if (ret == -EMSGSIZE) {
			resp->status = -EMSGSIZE;
			resp->data_len = 0;
			privsep_send_resp(sock, resp);
			continue;
		}
		if (ret < 0)
			break; /* garbled message: not a well-formed peer, stop */

		cmd.opcode = req->opcode;
		cmd.nsid = req->nsid;
		cmd.cdw10 = req->cdw10;
		cmd.cdw11 = req->cdw11;
		cmd.cdw12 = req->cdw12;
		cmd.cdw13 = req->cdw13;
		cmd.cdw14 = req->cdw14;
		cmd.cdw15 = req->cdw15;
		cmd.data_len = req->data_len;

		if (req->opcode == SPIKE_MOCK_WRITE_OPCODE)
			cmd.addr = (uint64_t)(uintptr_t)req->data;
		else
			cmd.addr = (uint64_t)(uintptr_t)resp->data;

		resp->status = libnvme_exec_admin_passthru(hdl, &cmd);
		resp->result = (uint32_t)cmd.result;
		resp->data_len = (req->opcode == SPIKE_MOCK_WRITE_OPCODE) ?
			0 : req->data_len;

		privsep_send_resp(sock, resp);
	}

	/*
	 * The loopback handle's "commands consumed" bookkeeping lives in
	 * this process's own (forked, COW) copy of *hdl -- only this
	 * process's calls to libnvme_exec_admin_passthru() above ever
	 * touched it. Asserting full consumption has to happen here, not
	 * in the parent, whose copy never changes.
	 */
	libnvme_loopback_end(hdl);
	libnvme_close(hdl);

	free(req);
	free(resp);
}
