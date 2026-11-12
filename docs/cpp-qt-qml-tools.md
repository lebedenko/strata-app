# Reusable C++ / Qt / QML checks

This pilot targets Arch Linux, C++23 and Qt 6. Repository checks never depend on this guide's location. Copy `tooling/`, the check tasks and tool configurations, then adapt source roots, tests, dependency roots and presets for each project.

## Installation

Install `base-devel cmake ninja git qt6-base qt6-declarative qt6-svg tomlplusplus go-task clang llvm clazy cppcheck gitleaks arch-audit actionlint valgrind ccache python python-pip` with pacman. On this workstation use `sudo -A pacman -S --needed ...`. Run `python3 -m venv .tooling-venv` and `.tooling-venv/bin/pip install -r tooling/requirements.txt`.

Install the official CodeQL CLI bundle, including the C++ query packs, separately; put `codeql` on PATH. This setup assumes a public repository and eligible CodeQL use. No GitHub credentials or uploads are used. Check applicable licensing before adapting to private code. Query packs must be available locally; a missing pack is an error. Version strings for every invoked tool are stored in reports. Use matching Clang, Clazy and LLVM tool versions; unsupported compiler/database options fail visibly.

## Commands and contracts

`task check-lvl1`, `task check-lvl2`, and `task check-lvl3` are cumulative and sequential. Each check executes once per batch. Run a single tool as `task check:clang-tidy` (likewise `clazy`, `cppcheck`, `asan-ubsan`, `codeql`, `valgrind`, `coverage`, `fuzz`, `dependencies`, `secrets-tree`, `secrets-history`, `clang-format`, `qmlformat`, `qmllint`, `cmake-format`, `cmake-lint`, `actionlint`, `build`, `tests`).

Level 1: C++/QML/CMake formatting, CMake lint, Actionlint, working-tree secrets, incremental warning-enabled build, generated Qt qmllint target, four headless tests. It needs no network. A warm run should aim below a minute; cold builds take longer.

Level 2 adds clang-tidy (including Clang Static Analyzer), Clazy levels 0–1, Cppcheck, ASan/UBSan with leak detection, installed project package auditing, and complete Git history secret scanning. Level 3 adds local CodeQL security-extended SARIF, Valgrind child-process tracing, LLVM line/branch coverage and decoder fuzzing.

Checks never format sources. `task format` remains the explicit C++ formatting action; use the configured Qt 6 qmlformat path and cmake-format explicitly when adopting changes. Build/test/install tasks remain compatible. CMake presets isolate dev, analysis, release, asan-ubsan, coverage and fuzz outputs below `build/`. Release has no runtime instrumentation. Qt tools are exported from the configured Qt6 imported targets; Qt 5 wrappers on PATH are not used. Analyzer inputs come from application and test entries in the compilation database after the generated metadata build; generated translation units are excluded.

Defaults are two build jobs (`CHECK_JOBS` overrides). `python3 tooling/checks.py lvl3 --timeout 1800 --fuzz-seconds 1800` exposes time budgets. Fuzzing uses seed 12345, persistent corpus/crashes in `.cache/fuzz`, and key/sensor decoders with arbitrary UTF-8 and 32-bit parameters. Reproduce a crash with `build/fuzz/fuzz_decoder .cache/fuzz/crashes/<input>`. Coverage is informational; no percentage gate is imposed.

## Reports and interpretation

Each invocation writes `reports/<UTC timestamp>/summary.md`, schema-versioned `summary.json`, logs and native reports. `reports/latest` points to the most recent run. Reports include commit/dirty state, environment, durations and tool versions. Statuses are `passed`, `findings`, `error` (including absent tools/network/timeouts) and `blocked` for failed shared build prerequisites. Finding counts are null unless a native format supports reliable counting. Any nonpassed required check fails the batch. Independent checks continue after failures. A nonblocking filesystem lock rejects overlapping runs sharing this checkout. Do not run manual builds concurrently with a batch.

Gitleaks reports and console matches are fully redacted. Working-tree scanning copies tracked and nonignored untracked files to a temporary directory, excluding generated outputs; ignored source inputs require explicit inclusion in your project policy. History scans all local refs; CI fetches complete history. Keep reports private when investigating source/security findings even with secret redaction.

The dependency audit inventories installed package versions and recursively resolves declared Qt/toml++ pacman dependencies. Known Arch advisories in that closure fail; unrelated host advisories are reported separately. This is distribution-advisory coverage only, not proof of safety or an upstream vulnerability inventory. OSV is useful later for vendored/submodule/lockfile dependencies; Trivy for deployment images. Neither reconstructs this pacman dependency closure from CMake.

## Review and exceptions

Fix actionable findings before adoption. Suppress only a demonstrated false positive or external-library issue, with exact rule/symbol/stack, reason and review context. Avoid blanket baselines, directory-wide analyzer exclusions, or disabling leak detection. No external-library leak suppressions are supplied without evidence. Tool failures are never converted into successful skips. Sanitizers, coverage and fuzzing cover executed paths and do not instrument distribution Qt libraries. Valgrind is slower and may expose external-library defects; document narrow stack suppressions if needed.

Clang Static Analyzer runs through clang-tidy: do not duplicate it with scan-build. MSan needs instrumented dependencies; TSan needs a meaningful threading workload; ABI checks need a published library ABI. IWYU, Semgrep, complexity dashboards and profiling are deferred until justified by a pilot workload. None of these checks establishes that the application is vulnerability-free.

## Nightly LAN server example

Level 3 is local only. Use an isolated checkout on `ci.local` with two cores and 16 GB RAM, persistent `.cache` and reports. CodeQL starts with one analysis thread and a 6144 MB analysis RAM budget. Measure wall time and peak RSS with `/usr/bin/time -v task check-lvl3`; this is process-tree accounting, not a complete system memory measurement. Do not overlap runs. Ensure the server has tools/query packs and the intended source revision before scheduling.

Example user service `~/.config/systemd/user/strata-checks.service` (replace paths):

```ini
[Unit]
Description=Strata nightly checks
[Service]
Type=oneshot
TimeoutStartSec=6h
WorkingDirectory=/srv/checkouts/strata-app
Environment=CHECK_JOBS=2
Environment=DEBUGINFOD_URLS=https://debuginfod.archlinux.org
Environment=PATH=/opt/codeql:/usr/local/bin:/usr/bin
ExecStart=/usr/bin/python3 tooling/checks.py lvl3
ExecStopPost=/usr/bin/find /srv/checkouts/strata-app/reports -mindepth 1 -maxdepth 1 -type d -name '20??????T*Z' -mmin +20160 -exec /usr/bin/rm -rf -- {} +
```

Example timer `strata-checks.timer`:

```ini
[Unit]
Description=Run Strata checks nightly
[Timer]
OnCalendar=*-*-* 03:00:00 Europe/Kyiv
Persistent=true
Unit=strata-checks.service
[Install]
WantedBy=timers.target
```

Install examples manually, then `systemctl --user daemon-reload` and `systemctl --user enable --now strata-checks.timer`. Use `loginctl enable-linger` only if desired and authorized. Systemd prevents concurrent starts of the same service; the runner lock additionally protects manual invocations. Retention removes only timestamped report directories; corpus and compiler caches persist. No service, timer, self-hosted GitHub runner or remote status integration is installed by this repository.

### Reviewed rule selection

The original clang-tidy bugprone, modernize, performance and readability families remain enabled, with Clang analyzer checks added. Specific style-policy rules are excluded: trailing return types (existing declaration convention), magic numbers (HID/USB protocol tables and fixture values), identifier length (protocol names), converting methods to static (Qt metaobject/invokable surface), redundant access specifiers (Qt signals/slots sections), cognitive complexity (protocol switch tables), trailing commas (formatter convention), easily swappable parameters (fixed D-Bus method signatures), and enum size (Qt metaobject enums). All other enabled diagnostics are errors. These are rule-policy choices, not a finding baseline. Qt D-Bus stream operators carry local documented exceptions for returning their const stream argument, as required by Qt's marshalling contract.

Run `python3 -m unittest discover -s tooling -p 'test_*.py'` for report/timeout/prerequisite contracts and `python3 tooling/verify_fixtures.py` for real formatting, redacted secret, analyzer-null-dereference and ASan failure fixtures. Fixtures use temporary directories and do not alter application sources.

CodeQL's `cpp/path-injection` result at the exact `std::ofstream out(configPath_);` call has a reviewed exception in `tooling/codeql-exceptions.json`: this unprivileged desktop process intentionally honors the same user's XDG_CONFIG_HOME/HOME. The rule, source file and exact primary source line must all match. Raw SARIF remains intact; `codeql-reviewed.sarif` records accepted suppressions with their reasons. Re-review this exception if privilege or remote-input boundaries change. Cppcheck receives the actual generated Qt MOC revision macro because its Qt library model does not define the header macro used by generated test metadata. Memcheck starts the private bus outside instrumentation and traces the test executable's children; per-process logs avoid child output overwriting parent reports. Instrumented lifecycle waits use an explicit longer budget.

The runner also records a source digest, checks that tracked/nonignored source inputs remain unchanged, marks incomplete/interrupted runs explicitly, and saves a Linux child-process peak RSS value. This RSS is the largest child high-water mark, not simultaneous host-wide usage. Sanitizer and fuzz presets build only the required tests/harness; coverage also builds the instrumented application. Analysis generates all application/test AUTOMOC metadata through CMake’s global autogen target, avoiding another plain C++ compilation before the analyzers. Qt 6.12 / Valgrind 3.25.1 exceptions and their observed stacks are documented in `tooling/suppressions/README.md`. They do not suppress leaks; all traced processes must produce complete Memcheck summaries.


For a non-login Arch server session, export `DEBUGINFOD_URLS=https://debuginfod.archlinux.org` before running Valgrind, or install matching debug symbols locally. Missing mandatory loader symbols cause a tool error, not a successful scan. Keep the debuginfod cache persistent; an uncached Memcheck run can need network access. The local level 1 batch never uses this service.

The CodeQL exception also pins the SHA-256 of `config_manager.cpp`; any change to that file requires a fresh review. Dependency findings are intentionally unsuppressed: the Arch tracker can retain unresolved entries after upstream fixes, so the native report must be compared with the recorded installed package versions before treating a group as a confirmed vulnerability. For example, [PAM 1.7.1](https://github.com/linux-pam/linux-pam/releases/tag/v1.7.1) fixes CVE-2025-6020, while the [Arch xdg-email advisory](https://security.archlinux.org/AVG-1420) remains unresolved. Do not solve a failing audit by creating a blanket baseline or silently ignoring failed network requests.


Coverage includes the instrumented application binary even when tests never execute it, so application-only sources appear with zero hits. Coverage and sanitizer helpers exit gracefully, and the lifecycle parent checks their exit status and forwards output; normal development tests retain abrupt daemon-stop coverage. Counts represent unsuppressed findings; successful enforced checks report zero, while build/test/coverage metrics and uncounted diagnostics use null.


Coverage runs the instrumented application's `--version` path offscreen before the four CTest tests, registering its distinct profile hashes and exercising argument handling. Native JSON captures stdout only; LLVM diagnostics remain in the log. LLVM may report unmatched empty Qt metadata thunks when combining binaries; retain those diagnostics when interpreting this informational report.


When updating an existing server checkout by copying files, do not preserve timestamps older than its build outputs: use `rsync --no-times` for changed source files or clean the affected presets first. Header ABI changes combined with stale objects can produce crashes even when the source fingerprint matches. The pilot verified the final server sources with fresh build artifacts after detecting this cache issue.

The LAN pilot's first full nightly run took 2 h 39 min and peaked at 4.05 GiB
largest-child RSS. The workstation warm level 1 run took 2.086 s. Final server
checks passed apart from six unsuppressed distribution-advisory groups; detailed
results and limitations are in `docs/tooling-validation.md` in the repository.
The server's stripped QtCore fallback is enabled only for its reviewed binary
SHA-256, and changing the library disables it automatically. See
`tooling/suppressions/README.md` before adapting any exceptions.
