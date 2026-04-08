#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of nvme.
# Copyright (c) 2026 SUSE LLC
#
# Authors: Daniel Wagner <dwagner@suse.de>
"""
TAP (Test Anything Protocol) version 13 runner for nvme-cli Python tests.

Wraps Python's unittest framework and emits TAP output so that meson can
parse individual subtest results when protocol: 'tap' is set in meson.build.
"""

import argparse
import importlib
import io
import sys
import traceback
import unittest


class TAPTestResult(unittest.TestResult):
    """Collect unittest results and render them as TAP version 13."""

    def __init__(self, stream: io.TextIOBase, diag_stream: io.TextIOBase) -> None:
        super().__init__()
        self._stream = stream
        self._diag_stream = diag_stream
        self._test_count = 0

    def _description(self, test: unittest.TestCase) -> str:
        return '{} ({})'.format(test._testMethodName, type(test).__name__)

    def addSuccess(self, test: unittest.TestCase) -> None:
        super().addSuccess(test)
        self._test_count += 1
        self._stream.write('ok {} - {}\n'.format(
            self._test_count, self._description(test)))
        self._stream.flush()

    def addError(self, test: unittest.TestCase, err: object) -> None:
        super().addError(test, err)
        self._test_count += 1
        self._stream.write('not ok {} - {}\n'.format(
            self._test_count, self._description(test)))
        self._stream.flush()
        self._diag_stream.write(
            ''.join(traceback.format_exception(*err)))  # type: ignore[misc]
        self._diag_stream.flush()

    def addFailure(self, test: unittest.TestCase, err: object) -> None:
        super().addFailure(test, err)
        self._test_count += 1
        self._stream.write('not ok {} - {}\n'.format(
            self._test_count, self._description(test)))
        self._stream.flush()
        self._diag_stream.write(
            ''.join(traceback.format_exception(*err)))  # type: ignore[misc]
        self._diag_stream.flush()

    def addSkip(self, test: unittest.TestCase, reason: str) -> None:
        super().addSkip(test, reason)
        self._test_count += 1
        self._stream.write('ok {} - {} # SKIP {}\n'.format(
            self._test_count, self._description(test), reason))
        self._stream.flush()

    def addExpectedFailure(self, test: unittest.TestCase, err: object) -> None:
        super().addExpectedFailure(test, err)
        self._test_count += 1
        self._stream.write('ok {} - {} # TODO expected failure\n'.format(
            self._test_count, self._description(test)))
        self._stream.flush()

    def addUnexpectedSuccess(self, test: unittest.TestCase) -> None:
        super().addUnexpectedSuccess(test)
        self._test_count += 1
        self._stream.write('not ok {} - {} # TODO unexpected success\n'.format(
            self._test_count, self._description(test)))
        self._stream.flush()


def run_tests(test_module_name: str, start_dir: str | None = None) -> bool:
    if start_dir:
        sys.path.insert(0, start_dir)

    module = importlib.import_module(test_module_name)

    loader = unittest.TestLoader()
    suite = loader.loadTestsFromModule(module)

    real_stdout = sys.stdout
    real_stderr = sys.stderr
    # TAP version header and plan must appear before any test output.
    real_stdout.write('TAP version 13\n')
    real_stdout.write('1..{}\n'.format(suite.countTestCases()))
    real_stdout.flush()

    # Redirect sys.stdout to real_stderr so that print()/sys.stdout.write()
    # calls from setUp/tearDown/tests do not pollute the TAP stream on stdout.
    # All diagnostic output (print statements, subprocess stderr via fd 2, and
    # test failure tracebacks) goes to stderr so that 'meson test -v' shows it
    # live on the terminal.
    sys.stdout = real_stderr  # type: ignore[assignment]
    try:
        result = TAPTestResult(real_stdout, real_stderr)
        suite.run(result)
    finally:
        sys.stdout = real_stdout

    return result.wasSuccessful()


def main() -> None:
    parser = argparse.ArgumentParser(
        description='TAP test runner for nvme-cli tests')
    parser.add_argument('test_module', help='Test module name to run')
    parser.add_argument('--start-dir',
                        help='Directory to prepend to sys.path for imports',
                        default=None)
    args = parser.parse_args()

    success = run_tests(args.test_module, args.start_dir)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
