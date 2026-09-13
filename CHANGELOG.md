# Changelog

All notable changes to QPLC will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.2.0] — 2026-08-29

### Added — language
- `struct` / `enum` user-defined types with field access & struct literals
- `match` statement with literal/wildcard patterns (ladder + SCL codegen)
- `try` / `except` / `finally` / `raise` error handling
- String literals (`"..."`), `len`/`print`/`type` builtins, string slices
- Module system: `import "file.q" [as alias]` with recursive resolution
- Field assignment (`obj.field = expr`) and namespaced calls (`m.fn(...)`)

### Added — TIA Portal integration
- **QPLC.PlcSimAdapter**: Hardware-in-the-Loop bridge to Siemens PLCSIM
  Advanced (S7-1500, API v1.0–6.0). Native C bridge + .NET P/Invoke driver;
  runs QPLC scans synchronized with the virtual PLC's I/O.
- CMake auto-detect + build of `libqplc_plcsim_bridge.dll` when PLCSIM is
  installed (skipped otherwise)

### Added — platform/CI
- 34/34 integration tests (was 32)
- New examples: `struct_enum_test.q`, `match_test.q`, `plcsim_demo.q`
- GitHub Actions: CI matrix (Windows MSYS2 + Ubuntu + .NET), VitePress
  docs deploy to GitHub Pages, release workflow
- VitePress documentation incl. 10-chapter "QPLC Book" (Rust-Book style)

### Fixed
- Cross-platform `tests/run_tests.sh` (binary name `qplc` vs `qplc.exe`)
- CI: artifact passing between jobs; msys2 shell scoping
- `.zcode/` session artifacts excluded from the repository

## [0.1.0] — 2026-08-28

Initial public release with:
- Ladder + SCL codegen for timers, counters, edge detection
- WPF visual simulator
- Console REPL simulator
- 12 example programs
- 24-test shell test suite
