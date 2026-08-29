# QPLC — Industrial Ladder Logic DSL

[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-00599C.svg)](src)
[![.NET](https://img.shields.io/badge/.NET-8.0-512BD4.svg)](QPLC.Core)
[![Tests](https://img.shields.io/badge/tests-32%2F32-brightgreen.svg)](tests)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

> A modern Python-like DSL that compiles to **Ladder Logic** and **SCL** for Siemens S7‑1200 PLCs, with a high‑fidelity **simulator**, **Modbus TCP** server, **time‑travel debugger**, and a cross‑platform **Studio** IDE.

---

## ✨ Features

- 🐍 **Python-like syntax** — indentation, `if/while/for`, `def`, `return`, ternary `a if cond else b`
- ⚡ **Modern C++20 compiler** — recursive descent parser, semantic analyzer, DNF‑based ladder codegen
- 🏭 **Industrial I/O** — IEC timers (`TON/TOF/TP`), counters (`CTU/CTD/CTUD`), edge detection
- 📐 **IEC 61131‑3 math** — `MIN/MAX/ABS/LIMIT/SEL/MUX` (with `min/max/abs/clamp/sel/mux` aliases)
- 🪜 **Ladder export** — Ladder XML consumed by all simulators
- 📜 **SCL export** — directly importable into TIA Portal V19
- 🖥️ **3 simulators** — console REPL, WPF visualizer (legacy), Avalonia cross‑platform Studio
- 🔌 **Modbus TCP server** — connect SCADA/HMI to live simulation (FC 1/2/3/5/6/15/16)
- ⏪ **Time‑travel debug** — 1000‑scan circular snapshot buffer with replay
- 🌐 **Language Server Protocol** — `qplc --lsp` for VS Code diagnostics
- 🧪 **32 tests** — golden `test_all.q` + codegen + semantic + simulator

---

## 🚀 Quick Start

### Build (Windows / MSYS2 UCRT64)

```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake -S . -B build -G Ninja
cmake --build build
```

### Build (Linux)

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### Compile and run a sample

```bash
./build/qplc examples/conf.qplc examples/test_all.q -o output.xml
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc output.xml
```

Inside the REPL:

```
> set enable True
> run
> show
> tick 5000
> run
> trace 0
> exit
```

### Studio (cross‑platform GUI)

```bash
dotnet QPLC.Studio/bin/Debug/net8.0/QPLC.Studio.dll
```

Open a config and a ladder XML, toggle Modbus, run/step/reset, replay trace.

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  Source (.q) ──► qplc (C++20) ──► output.xml (Ladder)        │
│                                  └─► output.scl (TIA Portal) │
└────────────┬─────────────────────────────────────────────────┘
             │
             ▼
┌──────────────────────────────────────────────────────────────┐
│  QPLC.Core (.NET 8 class library)                            │
│  ├─ ConfigParser  (conf.qplc)                                │
│  ├─ LadderXmlParser                                          │
│  ├─ LadderSimulator   (timers, counters, IEC math, trace)    │
│  └─ ModbusServer    (FC 1/2/3/5/6/15/16)                     │
└────────────┬─────────────────────────────────────────────────┘
             │
   ┌─────────┴─────────┬──────────────┐
   ▼                   ▼              ▼
QPLCSimulator    QPLCVisualSim.   QPLC.Studio
(console REPL)   (WPF, legacy)    (Avalonia, modern)
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for details.

---

## 📦 Repository Layout

| Path | Purpose |
|------|---------|
| `src/` | C++20 compiler (lexer, parser, semantic, codegen) |
| `QPLC.Core/` | Shared .NET 8 class library (simulator + Modbus) |
| `QPLCSimulator/` | Console REPL simulator |
| `QPLCVisualSimulator/` | WPF visual simulator (legacy, deprecated) |
| `QPLC.Studio/` | Avalonia cross-platform IDE (recommended) |
| `examples/` | Sample QPLC programs + conf.qplc |
| `tests/` | Shell-based test suite (32 tests) |
| `docs/` | Language reference and architecture |
| `vscode/` | VS Code extension scaffold + TextMate grammar |
| `.github/workflows/` | CI matrix (Windows MSYS2 + Ubuntu) |

---

## 🌐 Language Server

Start the LSP server:

```bash
./build/qplc --lsp
```

Then connect any LSP client (VS Code, Neovim, Helix). The server publishes diagnostics on every text change.

---

## 🤝 Contributing

We welcome contributions! See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines, and [`ROADMAP.md`](ROADMAP.md) for upcoming work.

## 📄 License

Apache‑2.0 — see [`LICENSE`](LICENSE).

## 🔒 Security

See [`SECURITY.md`](SECURITY.md) for reporting vulnerabilities.
