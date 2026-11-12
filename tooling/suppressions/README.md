# Reviewed external-library exceptions

`qt6.valgrind.supp` contains two stack-specific exceptions observed on Arch Qt
6.12.0 with Valgrind 3.25.1. Neither suppresses leaks or application allocation
errors. Revisit these when Qt or Valgrind changes; raw unsuppressed evidence is in
the pilot run `20261112T174036.826982Z`.

- `qt612-dbus-metatype-aes-tail-read`: a 16-byte SIMD read inside Qt's
  `aeshash128`, reached through `findMetaTypeByName`, `decodeMethodSignature`, and
  `QDBusConnectionPrivate::findSlot`. The Qt-allocated signature buffer is one
  byte shorter than the SIMD read. This is accepted as an external Qt hash-path
  defect for this pilot, not an assertion that the read is correct. Only that
  exact library call chain and access size are suppressed. The application does
  not supply an invalid or freed buffer here. All lifecycle processes reported
  zero definite and indirect leaks in the unsuppressed run.
- `qt612-forkfd-linux-waitid-null-infop`: Qt's bundled forkfd probes the Linux
  waitid syscall with a null `infop` from QProcess startup. A minimal Qt-only
  program reproduces the report. Linux permits this, although POSIX requires a
  non-null pointer ([Linux manual](https://man7.org/linux/man-pages/man2/waitpid.2.html#BUGS)).
  Match only the `waitid(infop)` parameter and the forkfd/QProcess startup stack.

Clazy exceptions are attached to individual const D-Bus test slot declarations:
these getters intentionally must remain slots to be exported by the fake daemon.
Cppcheck's value-return exception is attached only to `Manager::GetDevices`:
Qt MOC cannot export its reference-return form as a D-Bus method.


The LAN server uses `qt6-base 6.12.0-4`, while the workstation uses `6.12.0-5`.
The server's QtCore debug symbols could not be obtained from debuginfod. Its
unsuppressed run `20261112T202913.759422Z` shows the same QtCore SIMD location
through D-Bus lookup, another QtCore-only chain through QObject::connect during
QGuiApplication initialization, and the same waitid probe. Every process had
zero definite/indirect leaks. `qt612-arch4-stripped.valgrind.supp` matches only
those access sizes/parameter and exact library/entry-point frame sequences.

The runner enables that file only when the library SHA-256 exactly matches
`valgrind-libraries.json`. A package/binary change disables the fallback until
reviewed again. It is not an object-directory or all-Qt suppression. The current
library fingerprint is recorded in each Memcheck summary.
