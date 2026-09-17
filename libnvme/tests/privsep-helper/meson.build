# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# The Phase 1 privsep helper (issue #3879). Built only under -Dprivsep=true.
# Not installed: nothing wires it into normal nvme-cli usage yet (Phase 3),
# and it has no hardening yet (Phase 4).

privsep_helper = executable(
    'test-privsep-helper',
    ['helper.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
    ],
)
