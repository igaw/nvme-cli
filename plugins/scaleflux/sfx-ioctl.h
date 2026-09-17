/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of nvme-cli.
 *
 * ScaleFlux-specific raw ioctl numbers shared between sfx-nvme.c and the
 * privsep helper's ioctl allowlist (issue #3879 Phase 5) -- pulled out of
 * sfx-nvme.c so the two don't carry two independent copies of the same
 * magic number that could silently drift apart.
 */
#pragma once

#include <sys/ioctl.h>

#define NVME_IOCTL_CLR_CARD	_IO('N', 0x47)
