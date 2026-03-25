// SPDX-License-Identifier: GPL-2.0-or-later
#include <signal.h>
#include <errno.h>
#include <stddef.h>

#include "sighdl.h"

bool nvme_signal_received;

static void nvme_sigint_handler(int signum)
{
	nvme_signal_received = true;
}

int nvme_install_signal_handler(void)
{
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_handler = nvme_sigint_handler;
	act.sa_flags = 0;

	nvme_signal_received = false;
	if (sigaction(SIGINT, &act, NULL) == -1)
		return -errno;

	if (sigaction(SIGTERM, &act, NULL) == -1)
		return -errno;

	return 0;
}
