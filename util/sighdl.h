/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __NVME_SIGHDL
#define __NVME_SIGHDL

#include <stdbool.h>

extern bool nvme_signal_received;

int nvme_install_signal_handler(void);

#endif // __NVME_SIGHDL
