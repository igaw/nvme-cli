# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Verify that each public header in nvme/*.h is self-sufficient:
# it can be included as the first and only header without any
# prior includes.  If compilation fails, a header is missing its
# own dependencies.

foreach hdr : [
    'cmds',
    'fabrics',
    'filters',
    'ioctl',
    'linux',
    'log',
    'mi',
    'nbft',
    'ops',
    'tree',
    'types',
    'util',
]
    exe = executable(
        'test-header-' + hdr,
        'test-header-' + hdr + '.c',
        dependencies: [libnvme_dep],
    )
    test('libnvme - header/' + hdr, exe)
endforeach
