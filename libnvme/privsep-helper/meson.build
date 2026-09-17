# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This file is part of nvme-cli.
#
# The privsep helper (issue #3879). Built and installed whenever
# -Dprivsep=true, independent of -Dtests -- this is a real production
# binary nvme-cli's own src/privsep-lifecycle.c spawns, not a test
# fixture (Phase 6). Requires libseccomp/libcap, both mandatory once
# -Dprivsep=true (see root meson.build).
#
# Links libnvme_internal_dep (static, full internal-linkage visibility --
# see libnvme/src/meson.build), not libnvme_dep: helper.c's fabrics
# support calls __nvmf_add_ctrl() (private-fabrics.h), which is
# deliberately not part of the public, version-scripted libnvme3.so.
# Statically embedding libnvme's implementation here means this binary
# has no runtime dependency on any libnvme .so at all, and no public API
# had to grow just to give this one caller internal access.
#
# ioctl-allowlist.{c,h} include plugins/scaleflux/sfx-ioctl.h for
# NVME_IOCTL_CLR_CARD, so anything that compiles it needs that directory
# on the include path.

scaleflux_inc = include_directories('../../plugins/scaleflux')

nvme_privsep_helper = executable(
    'nvme-privsep-helper',
    ['helper.c', 'allowlist.c', 'harden.c', 'ioctl-allowlist.c'],
    include_directories: scaleflux_inc,
    dependencies: [
        config_dep,
        ccan_dep,
        libnvme_internal_dep,
        libseccomp_dep,
        libcap_dep,
    ],
    install: true,
    install_dir: sbindir,
)

if want_tests
    subdir('tests')
endif
