#!/usr/bin/env python3
"""Exercise real tool failure contracts without modifying project sources."""
import json
import os
from pathlib import Path
import subprocess
import tempfile


def run(args, **kwargs):
    return subprocess.run(args, capture_output=True, text=True, timeout=60, **kwargs)


with tempfile.TemporaryDirectory(prefix='strata-check-fixtures-') as tmp:
    directory = Path(tmp)
    bad_format = directory / 'format.cpp'
    bad_format.write_text('int main(){return 0;}\n')
    assert run(['clang-format', '--dry-run', '--Werror', str(bad_format)]).returncode != 0
    secret = 'ghp_' + 'D3f9aBcK8mN2pQrS7tUvW4xY6zA1bC5dE0fG' 
    (directory / 'secret.txt').write_text('github_token = "' + secret + '"\n')
    native = directory / 'secrets.json'
    result = run(['gitleaks', 'dir', '--redact=100', '--report-format=json',
                  f'--report-path={native}', str(directory)])
    assert result.returncode == 1 and json.loads(native.read_text())
    assert secret not in result.stdout + result.stderr + native.read_text()
    defect = directory / 'defect.cpp'
    defect.write_text('int f() { int *p = nullptr; return *p; }\n')
    result = run(['clang-tidy', '--config={Checks: "-*,clang-analyzer-*", WarningsAsErrors: "*"}',
                  str(defect), '--', '-std=c++23'])
    assert result.returncode != 0 and 'clang-analyzer-core.NullDereference' in result.stdout
    defect.write_text('int main(int argc, char**) { auto *p = new int[1]; p[argc] = 42; delete[] p; }\n')
    binary = directory / 'sanitizer'
    assert run(['clang++', '-g', '-O0', '-fsanitize=address,undefined', str(defect), '-o', str(binary)]).returncode == 0
    result = run([str(binary)], env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1'})
    assert result.returncode != 0 and 'AddressSanitizer' in result.stderr
    print('Formatting, redacted secrets, analyzer defect, and sanitizer error fixtures passed.')
