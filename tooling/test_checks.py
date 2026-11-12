"""Contract tests for reports, prerequisites, timeouts and selection."""
import contextlib
import io
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from checks import CheckFailure, LEVELS, Runner


class RunnerTests(unittest.TestCase):
    def test_cumulative_checks_are_unique(self):
        for level in range(1, 4):
            names = sum(LEVELS[:level], [])
            self.assertEqual(len(names), len(set(names)))

    def test_failure_does_not_stop_independent_checks(self):
        with tempfile.TemporaryDirectory() as directory:
            runner = Runner(Path(directory))
            def check(name):
                if name == 'bad':
                    raise CheckFailure('findings', 'fixture', 1)
                if name == 'blocked':
                    raise CheckFailure('blocked', 'prerequisite')
            runner.check = check
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(runner.execute(['bad', 'blocked', 'good']), 1)
            summary = json.loads((Path(directory) / 'summary.json').read_text())
            self.assertEqual(summary['schema_version'], 1)
            self.assertEqual(summary['checks']['good']['status'], 'passed')
            self.assertEqual(summary['checks']['good']['finding_count'], 0)
            self.assertEqual(summary['checks']['bad']['finding_count'], 1)
            self.assertEqual(summary['checks']['blocked']['status'], 'blocked')

    def test_timeout_and_missing_tools_are_errors(self):
        with tempfile.TemporaryDirectory() as directory:
            runner = Runner(Path(directory), timeout=0.01)
            runner.log = io.StringIO()
            with self.assertRaises(CheckFailure) as error:
                runner.run(['python3', '-c', 'import time; time.sleep(10)'])
            self.assertEqual(error.exception.status, 'error')
            with patch('checks.shutil.which', return_value=None):
                with self.assertRaises(CheckFailure) as error:
                    runner.tool('missing-fixture-command')
            self.assertEqual(error.exception.status, 'error')

    def test_scanner_failure_and_interrupt_retain_reports(self):
        with tempfile.TemporaryDirectory() as directory:
            runner = Runner(Path(directory))
            def check(name):
                if name == 'network':
                    runner.run(['python3', '-c', 'raise SystemExit(3)'])
                if name == 'interrupt':
                    raise KeyboardInterrupt
            runner.check = check
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(runner.execute(['network', 'interrupt', 'later']), 1)
            summary = json.loads((Path(directory) / 'summary.json').read_text())
            self.assertFalse(summary['complete'])
            self.assertEqual(summary['checks']['network']['status'], 'error')
            self.assertEqual(summary['checks']['interrupt']['status'], 'error')
            self.assertEqual(summary['checks']['later']['status'], 'blocked')
            self.assertTrue((Path(directory) / 'network.log').exists())

    def test_native_json_excludes_diagnostics(self):
        with tempfile.TemporaryDirectory() as directory:
            runner = Runner(Path(directory))
            runner.log = io.StringIO()
            output = Path(directory) / 'native.json'
            runner.run(['python3', '-c',
                        'import json, sys; print(json.dumps(dict(value=1))); print("warning", file=sys.stderr)'],
                       output=output)
            self.assertEqual(json.loads(output.read_text()), {'value': 1})
            self.assertIn('warning', runner.log.getvalue())

    def test_stripped_library_exception_requires_exact_binary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runner = Runner(root)
            runner.log = io.StringIO()
            library = root / 'libQt6Core.so'
            library.write_bytes(b'approved binary')
            policy = root / 'tooling/suppressions'
            policy.mkdir(parents=True)
            (policy / 'valgrind-libraries.json').write_text(json.dumps([{
                'library': str(library),
                'sha256': hashlib.sha256(library.read_bytes()).hexdigest(),
                'suppression': 'pinned.supp', 'reason': 'fixture'
            }]))
            with patch('checks.ROOT', root):
                self.assertEqual(len(runner.valgrind_suppressions()), 2)
                library.write_bytes(b'changed binary')
                self.assertEqual(len(runner.valgrind_suppressions()), 1)

    def test_failed_build_blocks_dependents(self):
        with tempfile.TemporaryDirectory() as directory:
            runner = Runner(Path(directory))
            runner.prepared['dev'] = False
            with self.assertRaises(CheckFailure) as error:
                runner.prepare('dev')
            self.assertEqual(error.exception.status, 'blocked')


if __name__ == '__main__':
    unittest.main()
