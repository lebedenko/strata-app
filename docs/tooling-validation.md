# Tooling pilot validation — 2026-11-12

The C++/QML changes, check runner, presets and GitHub Actions workflow were
implemented and exercised on the workstation and an isolated checkout at
`ci.local:/home/andrii/strata-tooling-validation`. No schedule was installed.

## Results

| Run | Result | Wall time | Largest child RSS |
| --- | --- | --- | --- |
| Workstation warm `task check-lvl1` | All 9 checks passed | 2.086 s | Recorded in report |
| Workstation full level 3 | 18 passed; dependency audit findings | 38 min 47 s | 3.76 GiB |
| Workstation final level 2, metadata-only analyzer preparation | 14 passed; dependency audit findings | 124.390 s | 695 MiB |
| Initial full server nightly | All selected tools executed; findings drove subsequent fixes | 2 h 39 min 6 s | 4.05 GiB |
| Final server level 2 | 14 passed; dependency audit findings | 16 min 52 s | 727 MiB |
| Final server Memcheck | Passed with reviewed Qt exceptions; every process had zero definite/indirect leaks | 6 min 34 s | 448 MiB |
| Final server coverage | Passed; valid JSON and HTML, application-only sources included | 5 min 16 s | 514 MiB |

The workstation's full 30-minute fuzz run completed **62,055,731** executions;
the server's full run completed **8,903,589**. Neither produced a crash artifact.
Both used seed 12345. Corpus state differs between the persistent workstation
corpus and the initially seeded server corpus, so these are not throughput
benchmarks. A final server harness smoke run also passed.

The full workstation run was followed by verification of metadata-only analyzer
preparation from cleaned analysis artifacts. Application and harness source did
not change during that preparation optimization. Final server analysis and
runtime checks verified the corrected setup separately from the first full
nightly measurement. All accepted runs recorded intact source fingerprints.

## Remaining gate

The dependency audit deliberately remains nonzero for six Arch advisory groups:
`AVG-2901` (pam), `AVG-2898` (libxml2), `AVG-2885` (coreutils), `AVG-2765`
(openssl), `AVG-1420` (xdg-utils), and `AVG-2882` (openssl). They are not
suppressed. This is a distribution-tracker result, not a confirmation that every
reported group affects the installed version or this application. The report
retains installed versions, the project dependency closure, and other host
advisories separately. See the reusable guide for upstream-versus-distribution
limitations.

## Exceptions and verification

- Clang-tidy families and Clang Static Analyzer are enforced. Reviewed style
  choices and exact Qt stream/operator exceptions are documented in the guide.
- Clazy levels 0–1 and Cppcheck pass on application and test translation units.
  Only generated/external translation units are excluded.
- CodeQL security-extended passes with the exact config-path exception pinned to
  the source file SHA-256; raw and reviewed SARIF are both retained.
- Qt memory exceptions are documented in `tooling/suppressions/README.md`. The
  stripped-symbol server fallback requires the exact QtCore binary SHA-256.
  Changing that binary disables the fallback. No leak suppressions are used.
- Seven runner contract tests passed on both machines. Real temporary fixtures
  verified formatting, fully redacted Gitleaks output, an analyzer defect and an
  ASan error. A separate application memory-error fixture still failed with
  both Qt suppression files supplied.
- Missing executables, failed prerequisites, scanner errors, timeouts,
  interruption/report retention and concurrent-run rejection were checked.
- Coverage JSON was parsed and checked for the instrumented application entry
  point and zero-hit notifier code; helper processes flushed their profiles.
- Release compile commands were checked to exclude sanitizer and coverage flags.
  Presets and headless CTest execution were exercised in nested build directories.

Copying source while preserving timestamps initially reused stale objects after
getter signature changes. Clean rebuilding resolved the resulting server test
crashes. The guide documents clean-build or `rsync --no-times` requirements for
that workflow. Analysis now generates AUTOMOC metadata without a redundant
plain C++ compilation before the analyzers.

## Local evidence

Ignored report directories retain the full logs and native artifacts:

- `reports/20261112T191704.850252Z/`: completed workstation level 3.
- `reports/20261112T200805.677950Z/`: final metadata-only workstation level 2.
- `reports/20261112T195949.997871Z/`: warm level 1 measurement.
- `reports/server/initial-nightly/`: original server run and resource log; the
  large CodeQL database remains on the server.
- `reports/server/final-level2/`, `final-valgrind/`, `final-coverage/`, and
  `final-fuzz-smoke/`: accepted server follow-ups.

Experimental/aborted runs remain available for diagnosis and are not counted as
passing validation. GitHub Actions itself was not triggered; its workflow passed
Actionlint and invokes the same level 2 command verified on Arch Linux.
