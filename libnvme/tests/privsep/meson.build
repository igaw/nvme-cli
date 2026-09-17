# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# Phase 1 privsep test harness (issue #3879). Built only under
# -Dprivsep=true; drives libnvme/tests/privsep-helper/test-privsep-helper
# (built by the sibling subdir, see libnvme/tests/meson.build) through a
# real fork+exec+socketpair round trip against the NVME_TEST_FD/
# NVME_TEST_FD64 dry-run backend.

privsep_test = executable(
    'test-privsep',
    ['parent.c', 'util.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
    ],
)
test('libnvme - privsep', privsep_test, args: [privsep_helper.full_path()])
