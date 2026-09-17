# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# Pure, unit-tested modules from the privsep helper (issue #3879) --
# genuinely test-only, unlike the helper binary itself, so gated on
# want_tests independently of it (see ../meson.build).

parent_inc = include_directories('..')

test_privsep_allowlist = executable(
    'test-privsep-allowlist',
    ['test-allowlist.c', '../allowlist.c'],
    include_directories: parent_inc,
    dependencies: [
        config_dep,
        ccan_dep,
    ],
)
test('libnvme - privsep-allowlist', test_privsep_allowlist)

test_privsep_ioctl_allowlist = executable(
    'test-privsep-ioctl-allowlist',
    ['test-ioctl-allowlist.c', '../ioctl-allowlist.c'],
    include_directories: [parent_inc, scaleflux_inc],
    dependencies: [
        config_dep,
        ccan_dep,
    ],
)
test('libnvme - privsep-ioctl-allowlist', test_privsep_ioctl_allowlist)
