# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# The privsep helper (issue #3879). Built only under -Dprivsep=true.
# Not installed: nothing wires it into a real install path yet (Phase 6).
# Phase 4 hardening (allowlist.c, harden.c) requires libseccomp/libcap,
# both mandatory once -Dprivsep=true (see root meson.build).
#
# ioctl-allowlist.{c,h} (Phase 5) include plugins/scaleflux/sfx-ioctl.h
# for NVME_IOCTL_CLR_CARD, so anything that compiles it needs that
# directory on the include path.

scaleflux_inc = include_directories('../../../plugins/scaleflux')

privsep_helper = executable(
    'test-privsep-helper',
    ['helper.c', 'allowlist.c', 'harden.c', 'ioctl-allowlist.c'],
    include_directories: scaleflux_inc,
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

test_privsep_ioctl_allowlist = executable(
    'test-privsep-ioctl-allowlist',
    ['test-ioctl-allowlist.c', 'ioctl-allowlist.c'],
    include_directories: scaleflux_inc,
    dependencies: [
        config_dep,
        ccan_dep,
    ],
)
test('libnvme - privsep-ioctl-allowlist', test_privsep_ioctl_allowlist)
