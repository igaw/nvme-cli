# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# The privsep helper (issue #3879). Built only under -Dprivsep=true.
# Not installed: nothing wires it into a real install path yet (Phase 6).
# Phase 4 hardening (allowlist.c, harden.c) requires libseccomp/libcap,
# both mandatory once -Dprivsep=true (see root meson.build).

privsep_helper = executable(
    'test-privsep-helper',
    ['helper.c', 'allowlist.c', 'harden.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
        libseccomp_dep,
        libcap_dep,
    ],
)

test_privsep_allowlist = executable(
    'test-privsep-allowlist',
    ['test-allowlist.c', 'allowlist.c'],
    dependencies: [
        config_dep,
        ccan_dep,
    ],
)
test('libnvme - privsep-allowlist', test_privsep_allowlist)
