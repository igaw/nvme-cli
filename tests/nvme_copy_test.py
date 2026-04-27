# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of nvme-cli
#
# Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
#
# Authors: Arunpandian J <apj.arun@samsung.com>
#          Joy Gu <jgu@purestorage.com>

"""
NVMe Copy Testcase:-

    1. Issue copy command on set of block; shall pass.
    2. If cross-namespace copy formats are supported, enable and test
       cross-namespace copy formats.

"""

import subprocess

from nvme_test import TestNVMe, to_decimal


class TestNVMeCopy(TestNVMe):

    """
    Represents NVMe Copy testcase.
        - Attributes:
              - ocfs : optional copy formats supported
              - host_behavior_data : host behavior support data to restore during teardown
              - test_log_dir :  directory for logs, temp files.
    """

    def setUp(self):
        """ Pre Section for TestNVMeCopy """
        super().setUp()
        self.ocfs = self.get_ocfs()
        self.mcl = to_decimal(self.get_id_ns_field_value("mcl"))
        self.mssrl = to_decimal(self.get_id_ns_field_value("mssrl"))
        self.msrc = to_decimal(self.get_id_ns_field_value("msrc"))
        self.host_behavior_data = None
        cross_namespace_copy = self.ocfs & 0xc
        if cross_namespace_copy:
            # get host behavior support data (raw binary, use subprocess directly)
            get_features_cmd = f"{self.nvme_bin} get-feature {self.ctrl} " + \
                "--feature-id=0x16 --data-len=512 --raw-binary"
            result = subprocess.run(get_features_cmd, shell=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertEqual(result.returncode, 0, "ERROR : nvme get-feature failed")
            self.host_behavior_data = result.stdout
            # enable cross-namespace copy formats
            # bytes 4-5 of the Host Behavior Support Data Structure hold the flags
            current_flags = int.from_bytes(self.host_behavior_data[4:6], 'little')
            if current_flags & cross_namespace_copy:
                # skip if already enabled
                print("Cross-namespace copy already enabled, skipping set-features")
                self.host_behavior_data = None
            else:
                new_flags = current_flags | cross_namespace_copy
                data = (self.host_behavior_data[:4] +
                        new_flags.to_bytes(2, 'little') +
                        self.host_behavior_data[6:])
                set_features_cmd = f"{self.nvme_bin} set-feature " + \
                    f"{self.ctrl} --feature-id=0x16 --data-len=512"
                result = subprocess.run(set_features_cmd, shell=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                        input=data)
                self.assertEqual(result.returncode, 0, "Failed to enable cross-namespace copy formats")
        get_ns_id_cmd = f"{self.nvme_bin} get-ns-id {self.ns1}"
        result = self.run_cmd(get_ns_id_cmd)
        err = result.returncode
        self.assertEqual(err, 0, "ERROR : nvme get-ns-id failed")
        output = result.stdout
        self.ns1_nsid = int(output.strip().split(':')[-1])
        self.setup_log_dir(self.__class__.__name__)

    def tearDown(self):
        """ Post Section for TestNVMeCopy """
        if self.host_behavior_data:
            # restore saved host behavior support data (raw binary)
            set_features_cmd = f"{self.nvme_bin} set-feature {self.ctrl} " + \
                "--feature-id=0x16 --data-len=512"
            subprocess.run(set_features_cmd, shell=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           input=self.host_behavior_data)
        super().tearDown()

    def _check_format_supported(self, desc_format):
        """ Skip test if the given copy descriptor format is not supported """
        if not self.ocfs & (1 << desc_format):
            self.skipTest(f"descriptor format {desc_format} is not supported")

    def _check_ns_copy_limits(self):
        """ Skip test if namespace copy limits (mcl, mssrl, msrc) are not set """
        missing = [name for name, val in
                   [("mcl", self.mcl), ("mssrl", self.mssrl), ("msrc", self.msrc)]
                   if val == 0]
        if missing:
            self.skipTest(f"{', '.join(missing)} are 0, copy not supported on this namespace")

    def copy(self, sdlba, blocks, slbs, **kwargs):
        """ Wrapper for nvme copy
            - Args:
                - sdlba : destination logical block address
                - blocks : number of logical blocks (0-based)
                - slbs : source range logical block address
                - descriptor_format : copy descriptor format (optional)
                - snsids : source namespace id (optional)
                - sopts : source options (optional)
            - Returns:
                - None
        """
        desc_format = kwargs.get("descriptor_format", 0)
        # build copy command
        copy_cmd = f"{self.nvme_bin} copy {self.ns1} " + \
            f"--format={desc_format} --sdlba={sdlba} --blocks={blocks} " + \
            f"--slbs={slbs}"
        if "snsids" in kwargs:
            copy_cmd += f" --snsids={kwargs['snsids']}"
        if "sopts" in kwargs:
            copy_cmd += f" --sopts={kwargs['sopts']}"
        # run and assert success
        self.assertEqual(self.exec_cmd(copy_cmd), 0)

    def test_copy_format_0(self):
        """ Test copy with descriptor format 0 """
        self._check_format_supported(0)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=0)

    def test_copy_format_1(self):
        """ Test copy with descriptor format 1 """
        self._check_format_supported(1)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=1)

    def test_copy_format_2(self):
        """ Test copy with descriptor format 2 """
        self._check_format_supported(2)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=2, snsids=self.ns1_nsid)

    def test_copy_format_2_sopts(self):
        """ Test copy with descriptor format 2 and source options """
        self._check_format_supported(2)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=2, snsids=self.ns1_nsid, sopts=0)

    def test_copy_format_3(self):
        """ Test copy with descriptor format 3 """
        self._check_format_supported(3)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=3, snsids=self.ns1_nsid)

    def test_copy_format_3_sopts(self):
        """ Test copy with descriptor format 3 and source options """
        self._check_format_supported(3)
        self._check_ns_copy_limits()
        self.copy(0, 1, 2, descriptor_format=3, snsids=self.ns1_nsid, sopts=0)
