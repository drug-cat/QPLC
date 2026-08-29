# Changelog

All notable changes to QPLC will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- `return [expr]` statement (semantic + codegen)
- Python-style ternary operator: `trueExpr if cond else falseExpr`
- IEC 61131-3 math functions: `MIN/MAX/ABS/LIMIT/SEL/MUX` (with QPLC aliases `min/max/abs/clamp/sel/mux`)
- `QPLC.Core` shared class library (eliminates 3x duplication across simulators)
- Modbus TCP server in QPLC.Core (FC 1/2/3/5/6/15/16)
- Time-travel trace replay (1000-scan circular buffer)
- QPLC.Studio — cross-platform Avalonia IDE
- Language Server Protocol: `qplc --lsp`
- VS Code extension scaffold (TextMate grammar + language config)
- New examples: `return_test.q`, `ternary_test.q`, `stdlib_test.q`
- New tests (32 total, up from 24)

### Changed
- SCL codegen now normalizes math functions: `clamp → LIMIT(IN, MN, MX)`, etc.
- All simulators share QPLC.Core for parser/simulator/Modbus

### Removed
- ~900 lines of duplicated C# code (consolidated into QPLC.Core)

## [0.1.0] — 2026-08-28

Initial public release with:
- Ladder + SCL codegen for timers, counters, edge detection
- WPF visual simulator
- Console REPL simulator
- 12 example programs
- 24-test shell test suite
