// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Verify that <nvme/log.h> is self-sufficient: it can be included
 * as the very first (and only) header without any prior includes.
 */
#include <nvme/log.h>

int main(void) { return 0; }
