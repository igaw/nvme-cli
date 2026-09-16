# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# Throwaway Phase-0 spike for the privilege-separation design in GitHub
# issue #3879. Built only under -Dioctl-tests=true -Dprivsep-spike=true;
# not part of any default build or default `meson test` run.

privsep_spike = executable(
    'test-privsep-spike',
    ['parent.c', 'child.c', 'util.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
    ],
)
test('libnvme - privsep-spike', privsep_spike)

privsep_spike_bench = executable(
    'bench-privsep-spike',
    ['bench.c', 'child.c', 'util.c'],
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_test_dep,
    ],
)
# Informational latency/throughput sweep, not a correctness test -- built,
# but deliberately not registered with test() so it doesn't run under
# `meson test`. Run it directly: build/.../bench-privsep-spike
