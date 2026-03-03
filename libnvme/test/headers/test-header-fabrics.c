// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Verify that <nvme/fabrics.h> is self-sufficient: it can be included
 * as the very first (and only) header without any prior includes.
 */
#include <nvme/fabrics.h>

int main(void) { return 0; }
