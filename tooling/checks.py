#!/usr/bin/env python3
"""Sequential quality checks. Standard library only; every invocation writes a report."""
import argparse
import datetime as dt
import fcntl
import hashlib
import resource
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
LEVELS = [
    ['clang-format', 'qmlformat', 'cmake-format', 'cmake-lint', 'actionlint',
     'secrets-tree', 'build', 'qmllint', 'tests'],
    ['clang-tidy', 'clazy', 'cppcheck', 'asan-ubsan', 'dependencies', 'secrets-history'],
    ['codeql', 'valgrind', 'coverage', 'fuzz'],
]


class CheckFailure(Exception):
    def __init__(self, status, message, count=None):
        super().__init__(message)
        self.status, self.count = status, count


class Runner:
    def __init__(self, report, jobs=2, timeout=1800, fuzz_seconds=1800):
        self.report, self.jobs, self.timeout = report, jobs, timeout
        self.fuzz_seconds = fuzz_seconds
        self.results, self.versions, self.prepared = {}, {}, {}
        self.log = None
        self.started = time.monotonic()
        self.initial_sources = self.source_hashes()
        self.complete = False
        self.binary_fingerprints = {}

    def source_hashes(self):
        files = subprocess.check_output(['git', 'ls-files', '-co', '--exclude-standard', '-z'], cwd=ROOT).split(b'\0')
        return {os.fsdecode(file): hashlib.sha256((ROOT / os.fsdecode(file)).read_bytes()).hexdigest()
                for file in set(files) - {b''} if (ROOT / os.fsdecode(file)).is_file()}

    def tool(self, name, version='--version'):
        local = ROOT / '.tooling-venv/bin' / name
        path = str(local) if local.is_file() else shutil.which(name)
        if not path:
            raise CheckFailure('error', f'Missing executable: {name}')
        if path not in self.versions:
            result = subprocess.run([path, version], capture_output=True, text=True, timeout=15)
            value = (result.stdout + result.stderr).strip()
            if result.returncode or not value:
                raise CheckFailure('error', f'Cannot determine version of {name}: {value}')
            minimum = {'clang++': 18, 'clang-tidy': 18, 'clang-format': 18, 'gitleaks': 8}
            if name == 'gitleaks' and 'version is set by build process' in value and shutil.which('pacman'):
                package = subprocess.run(['pacman', '-Q', 'gitleaks'], capture_output=True, text=True, timeout=15)
                if package.returncode == 0:
                    value += '\n' + package.stdout.strip()
            match = re.search(r'(?<!\d)(\d+)\.\d+', value)
            if name in minimum and (not match or int(match[1]) < minimum[name]):
                raise CheckFailure('error', f'Incompatible {name}: {value}')
            self.versions[path] = value
        return path

    def run(self, argv, *, env=None, output=None, findings=False, timeout=None):
        self.log.write('$ ' + shlex.join(map(str, argv)) + '\n')
        self.log.flush()
        process = subprocess.Popen(list(map(str, argv)), cwd=ROOT,
                                   env={**os.environ, **(env or {})}, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True, start_new_session=True)
        try:
            data, diagnostics = process.communicate(timeout=timeout or self.timeout)
        except KeyboardInterrupt:
            os.killpg(process.pid, signal.SIGKILL)
            data, diagnostics = process.communicate()
            self.log.write(data + diagnostics)
            raise
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            data, diagnostics = process.communicate()
            self.log.write(data + diagnostics)
            raise CheckFailure('error', f'Timed out: {argv[0]}') from None
        self.log.write(data + diagnostics)
        self.log.flush()
        if output:
            Path(output).write_text(data)
        data += diagnostics
        if process.returncode:
            infrastructure_error = any(marker in data for marker in (
                'Traceback (most recent call last)', 'Fatal error at startup:',
                'unrecognized command line option', 'unknown argument:', 'unknown command line argument'))
            raise CheckFailure('findings' if findings and process.returncode > 0 and not infrastructure_error else 'error',
                               f'Exit {process.returncode}: {argv[0]}')
        return data

    def prepare(self, preset):
        if preset in self.prepared:
            if not self.prepared[preset]:
                raise CheckFailure('blocked', f'Failed prerequisite: {preset} build')
            return ROOT / 'build' / preset
        self.prepared[preset] = False
        self.tool('cmake')
        self.tool('ninja')
        self.tool('clang++')
        if shutil.which('ccache'):
            self.tool('ccache')
        self.run(['cmake', '--preset', preset, *(['-DCMAKE_CXX_COMPILER_LAUNCHER=ccache'] if shutil.which('ccache') else [])], env={'CCACHE_DIR': str(ROOT / '.cache/ccache')})
        targets = ['--target', 'test_decoder', 'test_config', 'test_layout_model', 'test_lifecycle'] if preset == 'asan-ubsan' else (['--target', 'fuzz_decoder'] if preset == 'fuzz' else (['--target', 'autogen'] if preset == 'analysis' else []))
        self.run(['cmake', '--build', '--preset', preset, '--parallel', self.jobs, *targets], env={'CCACHE_DIR': str(ROOT / '.cache/ccache')})
        self.prepared[preset] = True
        return ROOT / 'build' / preset

    def qt(self, name):
        build = self.prepare('dev')
        tool = json.loads((build / 'qt-tools.json').read_text())[name]
        version = self.run([tool, '--version'])
        if not re.search(r'\b6\.\d+', version):
            raise CheckFailure('error', f'{name} must be Qt 6: {version}')
        self.versions[tool] = version.strip()
        return tool

    def sources(self, build):
        entries = json.loads((build / 'compile_commands.json').read_text())
        selected = {}
        for entry in entries:
            file = Path(entry['file']).resolve()
            if any(file.is_relative_to(ROOT / directory) for directory in ('src', 'tests')):
                selected[str(file)] = entry
        if not all(any(Path(p).is_relative_to(ROOT / directory) for p in selected)
                   for directory in ('src', 'tests')):
            raise CheckFailure('error', 'Compilation database lacks application or test inputs')
        path = self.report / 'compilation-database'
        path.mkdir(exist_ok=True)
        (path / 'compile_commands.json').write_text(json.dumps(list(selected.values()), indent=2))
        return selected, path

    def moc_revision(self, build):
        # Qt headers define this macro for real compilation; Cppcheck's Qt model does not.
        moc = build / 'test_lifecycle_autogen/include/test_lifecycle.moc'
        match = re.search(r'Q_MOC_OUTPUT_REVISION != (\d+)', moc.read_text())
        if not match:
            raise CheckFailure('error', 'Cannot resolve the installed Qt MOC revision')
        return match[1]

    def valgrind_suppressions(self):
        directory = ROOT / 'tooling/suppressions'
        selected = [directory / 'qt6.valgrind.supp']
        for entry in json.loads((directory / 'valgrind-libraries.json').read_text()):
            library = Path(entry['library'])
            if not library.is_file():
                continue
            digest = hashlib.sha256(library.read_bytes()).hexdigest()
            self.binary_fingerprints[str(library)] = digest
            if digest == entry['sha256']:
                selected.append(directory / entry['suppression'])
                self.log.write('Applying pinned library exception: ' + entry['reason'] + '\n')
        return selected

    def tests(self, preset, env=None):
        self.tool('ctest')
        build = self.prepare(preset)
        listing = json.loads(self.run(['ctest', '--test-dir', build, '--show-only=json-v1']))
        if len(listing['tests']) != 4:
            raise CheckFailure('error', 'Expected all four CTest tests')
        self.run(['ctest', '--test-dir', build, '--output-on-failure', '--output-junit',
                  self.report / f'{preset}-tests.xml'], env=env, findings=True)

    def check(self, name):
        cpp = sorted(str(p) for directory in ('src', 'tests', 'fuzz')
                     for p in (ROOT / directory).rglob('*') if p.suffix in ('.cpp', '.hpp'))
        qml = sorted((ROOT / 'qml').rglob('*.qml'))
        if name == 'clang-format':
            if not cpp:
                raise CheckFailure('error', 'No C++ formatting inputs')
            self.run([self.tool('clang-format'), '--dry-run', '--Werror', *cpp], findings=True)
        elif name == 'qmlformat':
            if not qml:
                raise CheckFailure('error', 'No QML formatting inputs')
            tool = self.qt('qmlformat')
            changed = []
            for path in qml:
                if self.run([tool, path]) != path.read_text():
                    changed.append(str(path.relative_to(ROOT)))
            if changed:
                raise CheckFailure('findings', 'QML formatting differs: ' + ', '.join(changed), len(changed))
        elif name in ('cmake-format', 'cmake-lint'):
            self.run([self.tool(name), *(['--check'] if name == 'cmake-format' else []),
                      'CMakeLists.txt'], findings=True)
        elif name == 'actionlint':
            self.run([self.tool('actionlint', '-version'), '-color'], findings=True)
        elif name.startswith('secrets-'):
            tool = self.tool('gitleaks', 'version')
            # Redact both native JSON and console output; never store raw matches.
            path = self.report / f'{name}.json'
            mode = 'git' if name.endswith('history') else 'dir'
            argv = [tool, mode, '--redact=100', '--report-format=json',
                    f'--report-path={path}', '--exit-code=10']
            if mode == 'dir':
                # A clean staging copy includes ignored-free working-tree files, including untracked files.
                import tempfile
                with tempfile.TemporaryDirectory(prefix='strata-secrets-') as tmp:
                    files = subprocess.check_output(['git', 'ls-files', '-co', '--exclude-standard', '-z'], cwd=ROOT).split(b'\0')
                    for raw in set(files) - {b''}:
                        file = Path(os.fsdecode(raw))
                        source = ROOT / file
                        if source.is_file() and not source.is_symlink():
                            target = Path(tmp) / file
                            target.parent.mkdir(parents=True, exist_ok=True)
                            shutil.copyfile(source, target)
                    self.secret_scan([*argv, tmp], path)
            else:
                self.secret_scan([*argv, '--log-opts=--all', '.'], path)
        elif name == 'build':
            self.prepare('dev')
        elif name == 'qmllint':
            self.qt('qmllint')
            try:
                data = self.run(['cmake', '--build', ROOT / 'build/dev', '--target', 'all_qmllint',
                                 '--parallel', self.jobs], findings=True)
            except CheckFailure:
                # Qt can omit diagnostics at MaxWarnings=0; rerun only failures for native JSON.
                try:
                    self.run(['cmake', '--build', ROOT / 'build/dev', '--target', 'all_qmllint_json',
                              '--parallel', self.jobs], findings=True)
                finally:
                    for path in (ROOT / 'build/dev').rglob('*qmllint*.json'):
                        shutil.copyfile(path, self.report / path.name)
                raise
            if re.search(r'^(Warning|Error):', data, re.M):
                raise CheckFailure('findings', 'qmllint diagnostics; see log')
        elif name == 'tests':
            self.tests('dev')
        elif name in ('clang-tidy', 'clazy', 'cppcheck'):
            build = self.prepare('analysis')
            files, database = self.sources(build)
            if name == 'cppcheck':
                tool = self.tool('cppcheck')
                self.run([tool, f'--project={database / "compile_commands.json"}', '--std=c++23',
                          '--library=qt', '--inline-suppr', '--enable=warning,performance,portability',
                          '-DQ_MOC_OUTPUT_REVISION=' + self.moc_revision(build),
                          '--error-exitcode=10', '--xml', '--xml-version=2',
                          f'--output-file={self.report / "cppcheck.xml"}'], findings=True)
            else:
                tool = self.tool('clang-tidy' if name == 'clang-tidy' else 'clazy-standalone')
                findings = []
                for index, file in enumerate(files):
                    try:
                        args = ['--checks=level0,level1'] if name == 'clazy' else ['--export-fixes=' + str(self.report / f'tidy-{index}.yaml')]
                        data = self.run([tool, '-p', database, *args, file], findings=True)
                        if name == 'clazy' and re.search(r'warning:.*\[(?:-W)?clazy-', data):
                            findings.append(file)
                    except CheckFailure as error:
                        if error.status != 'findings':
                            raise
                        findings.append(file)
                if findings:
                    raise CheckFailure('findings', f'{name} findings in {len(findings)} translation units')
        elif name == 'asan-ubsan':
            self.tests('asan-ubsan', {'STRATA_INSTRUMENTED': '1', 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                                    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})
        elif name == 'dependencies':
            self.dependencies()
        elif name == 'valgrind':
            self.tool('ctest')
            tool = self.tool('valgrind')
            build = self.prepare('dev')
            directory = self.report / 'valgrind'
            directory.mkdir()
            listing = json.loads(self.run(['ctest', '--test-dir', build, '--show-only=json-v1']))
            if len(listing['tests']) != 4:
                raise CheckFailure('error', 'Expected four tests for Memcheck')
            failed = []
            suppressions = [f'--suppressions={path}' for path in self.valgrind_suppressions()]
            for test in listing['tests']:
                command = test['command']
                # Start the private bus outside Valgrind; instrument the test and all its children.
                prefix = []
                if Path(command[0]).name == 'dbus-run-session':
                    prefix, command = command[:2], command[2:]
                try:
                    self.run([*prefix, tool, '--trace-children=yes', '--leak-check=full',
                              '--show-leak-kinds=definite,indirect',
                              '--errors-for-leak-kinds=definite,indirect', '--error-exitcode=10',
                              *suppressions,
                              f'--log-file={directory}/{test["name"]}-%p.log', *command],
                             env={'QT_QPA_PLATFORM': 'offscreen', 'STRATA_INSTRUMENTED': '1'},
                             timeout=self.timeout, findings=True)
                except CheckFailure as error:
                    if error.status != 'findings':
                        raise
                    failed.append(test['name'])
            logs = list(directory.glob('*.log'))
            if len(logs) < 4:
                raise CheckFailure('error', 'Missing Memcheck test logs')
            for log in logs:
                data = log.read_text()
                if not re.search(r'ERROR SUMMARY: \d+ errors', data):
                    raise CheckFailure('error', f'Incomplete Memcheck report: {log.name}')
                if re.search(r'ERROR SUMMARY: [1-9]', data):
                    failed.append(log.name)
            if failed:
                raise CheckFailure('findings', 'Memcheck errors: ' + ', '.join(failed))
        elif name == 'coverage':
            self.coverage()
        elif name == 'fuzz':
            build = self.prepare('fuzz')
            corpus = ROOT / '.cache/fuzz/corpus'
            corpus.mkdir(parents=True, exist_ok=True)
            for index, behavior in enumerate(['key_press', 'home_row_mo', 'hml', 'layer_tap', 'inc_dec_kp', 'scroll_encoder']):
                (corpus / f'seed-{index}').write_bytes(bytes.fromhex('1400070000000000') + behavior.encode())
            crashes = ROOT / '.cache/fuzz/crashes'
            crashes.mkdir(exist_ok=True)
            self.run([build / 'fuzz_decoder', corpus, '-seed=12345',
                      f'-artifact_prefix={crashes}/', f'-max_total_time={self.fuzz_seconds}',
                      '-timeout=10', '-max_len=4096'],
                     env={'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'},
                     findings=True, timeout=self.fuzz_seconds + 120)
        elif name == 'codeql':
            tool = self.tool('codeql', 'version')
            database = self.report / 'codeql-db'
            self.run([tool, 'database', 'create', database, '--language=cpp', '--threads=1', '--ram=6144',
                      '--command=cmake --workflow --preset codeql'], env={'CCACHE_DISABLE': '1'}, timeout=3600)
            sarif = self.report / 'codeql.sarif'
            self.run([tool, 'database', 'analyze', database, 'cpp-security-extended.qls',
                      '--format=sarif-latest', f'--output={sarif}', '--threads=1', '--ram=6144'], timeout=3600)
            reviewed = json.loads(sarif.read_text())
            exceptions = json.loads((ROOT / 'tooling/codeql-exceptions.json').read_text())
            count = 0
            for run in reviewed['runs']:
                for result in run.get('results', []):
                    accepted = False
                    for location in result.get('locations', []):
                        physical = location.get('physicalLocation', {})
                        file = physical.get('artifactLocation', {}).get('uri')
                        line = physical.get('region', {}).get('startLine', 0)
                        for exception in exceptions:
                            if result['ruleId'] != exception['rule'] or file != exception['file']:
                                continue
                            if hashlib.sha256((ROOT / file).read_bytes()).hexdigest() != exception['file_sha256']:
                                continue
                            source = (ROOT / file).read_text().splitlines()
                            if 0 < line <= len(source) and source[line - 1].strip() == exception['source']:
                                result['suppressions'] = [{'kind': 'external', 'status': 'accepted',
                                                           'justification': exception['reason']}]
                                accepted = True
                    count += int(not accepted)
            (self.report / 'codeql-reviewed.sarif').write_text(json.dumps(reviewed, indent=2))
            if count:
                raise CheckFailure('findings', 'CodeQL security findings', count)
        else:
            raise CheckFailure('error', f'Unknown check: {name}')

    def secret_scan(self, argv, path):
        try:
            self.run(argv)
        except CheckFailure as error:
            if str(error).startswith('Exit 10:') and path.exists():
                raise CheckFailure('findings', 'Secrets detected (redacted)', len(json.loads(path.read_text()))) from None
            raise
        if not path.exists():
            raise CheckFailure('error', 'Gitleaks did not produce its report')

    def dependencies(self):
        tool = self.tool('arch-audit')
        self.tool('pacman')
        roots = ['qt6-base', 'qt6-declarative', 'qt6-svg', 'tomlplusplus']
        installed = self.run(['pacman', '-Q'])
        (self.report / 'packages.txt').write_text(installed)
        closure, pending = set(), list(roots)
        while pending:
            package = pending.pop()
            if package in closure:
                continue
            closure.add(package)
            data = self.run(['pacman', '-Qi', package], env={'LC_ALL': 'C'})
            match = re.search(r'^Depends On\s*:\s*(.*(?:\n +.*)*)', data, re.M)
            if not match:
                raise CheckFailure('error', f'Cannot parse dependency list for {package}')
            for dep in match[1].split():
                if dep == 'None':
                    continue
                dep = re.split(r'[<>=]', dep)[0]
                # Resolve virtual dependencies against installed package providers.
                query = subprocess.run(['pacman', '-Q', dep], cwd=ROOT, capture_output=True, text=True)
                if query.returncode:
                    providers = self.run(['pacman', '-Qi'], env={'LC_ALL': 'C'})
                    matches = [block for block in providers.split('\n\n') if re.search(r'^Provides\s*:.*\b' + re.escape(dep) + r'(?:[=\s]|$)', block, re.M)]
                    if len(matches) != 1:
                        raise CheckFailure('error', f'Cannot resolve installed provider: {dep}')
                    dep = re.search(r'^Name\s*:\s*(\S+)', matches[0], re.M)[1]
                pending.append(dep)
        (self.report / 'project-packages.json').write_text(json.dumps(sorted(closure), indent=2))
        data = self.run([tool, '--json'])
        (self.report / 'arch-audit.json').write_text(data)
        advisories = json.loads(data)
        if not isinstance(advisories, list):
            raise CheckFailure('error', 'Unsupported arch-audit JSON schema')
        project, host = [], []
        for advisory in advisories:
            packages = advisory.get('packages')
            if not isinstance(packages, list) or not all(isinstance(p, str) for p in packages):
                raise CheckFailure('error', 'Unsupported arch-audit package record')
            (project if closure.intersection(packages) else host).append(advisory)
        (self.report / 'dependency-findings.json').write_text(json.dumps({'project': project, 'other_host': host}, indent=2))
        if project:
            raise CheckFailure('findings', 'Known distribution advisories in project dependencies', len(project))

    def coverage(self):
        profdata = self.tool('llvm-profdata'); cov = self.tool('llvm-cov')
        profiles = self.report / 'profiles'
        profiles.mkdir()
        build = self.prepare('coverage')
        coverage_env = {'STRATA_INSTRUMENTED': '1', 'QT_QPA_PLATFORM': 'offscreen',
                        'LLVM_PROFILE_FILE': str(profiles / '%p-%m.profraw')}
        # Register the application's own profile hashes, including its distinct main().
        self.run([build / 'strata', '--version'], env=coverage_env, findings=True)
        self.tests('coverage', coverage_env)
        raw = list(profiles.glob('*.profraw'))
        if not raw:
            raise CheckFailure('error', 'No coverage profiles generated')
        merged = self.report / 'coverage.profdata'
        self.run([profdata, 'merge', '-sparse', *raw, '-o', merged])
        binaries = sorted((ROOT / 'build/coverage').glob('test_*'))
        binaries = [p for p in binaries if p.is_file() and os.access(p, os.X_OK)]
        if len(binaries) != 4:
            raise CheckFailure('error', 'Expected four instrumented test executables')
        application = ROOT / 'build/coverage/strata'
        if not application.is_file():
            raise CheckFailure('error', 'Missing instrumented application for coverage')
        binaries.append(application)
        args = [binaries[0]]
        for binary in binaries[1:]:
            args += ['-object', binary]
        args += [f'-instr-profile={merged}', '-ignore-filename-regex=(/usr/|_autogen/|/build/)']
        self.run([cov, 'export', *args], output=self.report / 'coverage.json')
        exported = json.loads((self.report / 'coverage.json').read_text())
        filenames = {file['filename'] for data in exported['data'] for file in data['files']}
        if str(ROOT / 'src/main.cpp') not in filenames:
            raise CheckFailure('error', 'Coverage omits the application entry point')
        self.run([cov, 'show', *args, '-format=html', '-show-branches=count',
                  f'-output-dir={self.report / "coverage-html"}'])
        self.run([cov, 'report', *args], output=self.report / 'coverage.txt')

    def execute(self, names):
        self.results = {name: {'status': 'blocked', 'message': 'Not run yet',
                               'finding_count': None, 'duration_seconds': 0, 'log': None}
                        for name in names}
        self.save()
        interrupted = False
        for name in names:
            started = time.monotonic()
            with (self.report / f'{name}.log').open('w') as self.log:
                try:
                    self.check(name)
                    result = {'status': 'passed', 'finding_count': None if name in ('build', 'tests', 'coverage') else 0}
                except KeyboardInterrupt:
                    interrupted = True
                    result = {'status': 'error', 'message': 'Interrupted', 'finding_count': None}
                except CheckFailure as error:
                    result = {'status': error.status, 'message': str(error), 'finding_count': error.count}
                except Exception as error:
                    result = {'status': 'error', 'message': f'{type(error).__name__}: {error}', 'finding_count': None}
                if self.source_hashes() != self.initial_sources:
                    result = {'status': 'error', 'message': 'Source files changed during checks', 'finding_count': None}
                result['duration_seconds'] = round(time.monotonic() - started, 3)
                result['log'] = f'{name}.log'
                self.results[name] = result
                print(f'{name}: {result["status"]} ({result["duration_seconds"]}s)', flush=True)
                self.save()
            if interrupted:
                break
        self.complete = not interrupted
        self.save()
        return int(any(r['status'] != 'passed' for r in self.results.values()))

    def save(self):
        def git(*args):
            return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()
        summary = {'schema_version': 1, 'created_at': self.report.name,
                   'complete': self.complete,
                   'binary_fingerprints': self.binary_fingerprints,
                   'source_digest': hashlib.sha256(json.dumps(self.initial_sources, sort_keys=True).encode()).hexdigest(),
                   'duration_seconds': round(time.monotonic() - self.started, 3),
                   'child_peak_rss_kib': resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss,
                   'source_integrity': self.source_hashes() == self.initial_sources,
                   'commit': git('rev-parse', 'HEAD'), 'dirty': bool(git('status', '--porcelain')),
                   'environment': {'platform': platform.platform(), 'python': sys.version,
                                   'jobs': self.jobs}, 'tool_versions': self.versions,
                   'checks': self.results}
        temporary = self.report / 'summary.json.tmp'
        temporary.write_text(json.dumps(summary, indent=2) + '\n')
        temporary.replace(self.report / 'summary.json')
        (self.report / 'summary.md').write_text(
            '# Quality checks\n\n'
            f'Commit: `{summary["commit"]}`; dirty: {summary["dirty"]}.\n'
            f'Complete: {self.complete}; source integrity: {summary["source_integrity"]}.\n'
            f'Wall time: {summary["duration_seconds"]}s; child peak RSS: {summary["child_peak_rss_kib"]} KiB.\n\n' + '\n'.join(
            f'- {name}: **{r["status"]}** ({r["duration_seconds"]}s) {r.get("message", "")}'
            for name, r in self.results.items()) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('check', choices=['lvl1', 'lvl2', 'lvl3', *sum(LEVELS, [])])
    parser.add_argument('--jobs', type=int, default=int(os.environ.get('CHECK_JOBS', '2')))
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--fuzz-seconds', type=int, default=1800)
    args = parser.parse_args()
    if min(args.jobs, args.timeout, args.fuzz_seconds) < 1:
        parser.error('jobs and time budgets must be positive')
    reports = ROOT / 'reports'
    reports.mkdir(exist_ok=True)
    with (reports / '.lock').open('w') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            parser.exit(2, 'Another check run owns the build/report lock\n')
        stamp = dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
        report = reports / stamp
        report.mkdir()
        latest = reports / 'latest'
        latest.unlink(missing_ok=True)
        latest.symlink_to(report.name, target_is_directory=True)
        names = sum(LEVELS[:int(args.check[-1])], []) if args.check.startswith('lvl') else [args.check]
        print(f'Report: {report}', flush=True)
        def interrupt_run(_signal, _frame):
            raise KeyboardInterrupt
        signal.signal(signal.SIGTERM, interrupt_run)
        try:
            runner = Runner(report, args.jobs, args.timeout, args.fuzz_seconds)
        except (OSError, subprocess.SubprocessError) as error:
            summary = {'schema_version': 1, 'complete': False, 'commit': None, 'dirty': None,
                       'checks': {'preflight': {'status': 'error', 'finding_count': None,
                                               'message': f'Git/source preflight: {error}'}}}
            (report / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
            (report / 'summary.md').write_text('# Preflight error\n\n' + str(error) + '\n')
            return 1
        return runner.execute(names)


if __name__ == '__main__':
    sys.exit(main())
