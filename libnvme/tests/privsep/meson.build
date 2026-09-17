# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# Phase 1 privsep test harness (issue #3879). Built only under
# -Dprivsep=true; drives the real, installed nvme-privsep-helper
# (libnvme/privsep-helper/, declared as nvme_privsep_helper -- Phase 6
# relocated it out of tests/ since it's a production binary, not a test
# fixture) through a real fork+exec+socketpair round trip against the
# NVME_TEST_FD/NVME_TEST_FD64 dry-run backend.

privsep_test = executable(
    'test-privsep',
    ['parent.c', 'util.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
    ],
)
test('libnvme - privsep', privsep_test, args: [nvme_privsep_helper])
