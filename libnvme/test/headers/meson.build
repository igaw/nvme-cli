# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Verify that each public header in nvme/*.h is self-sufficient:
# it can be included as the first and only header without any
# prior includes.  If compilation fails, a header is missing its
# own dependencies.
#
# The .c source files are generated at configure time from the
# test-header.c.in template so there is no per-header boilerplate
# to maintain in the repository.

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
    hdr_conf = configuration_data()
    hdr_conf.set('HDR', hdr)
    src = configure_file(
        input: 'test-header.c.in',
        output: 'test-header-' + hdr + '.c',
        configuration: hdr_conf,
    )
    exe = executable(
        'test-header-' + hdr,
        src,
        dependencies: [libnvme_dep],
    )
    test('libnvme - header/' + hdr, exe)
endforeach
