# QPLC Roadmap

This is the 12-month roadmap for the QPLC project. See [`CHANGELOG.md`](CHANGELOG.md) for what has already shipped.

## ✅ Phase 1 — Foundation (current, in this release)

| Area | Status |
|------|--------|
| Modern C++20 compiler (return, ternary, IEC math) | ✅ shipped |
| `QPLC.Core` shared class library | ✅ shipped |
| Modbus TCP server in Core | ✅ shipped |
| Time-travel trace replay in Core | ✅ shipped |
| QPLC.Studio (Avalonia cross-platform) | ✅ shipped |
| `qplc --lsp` Language Server | ✅ shipped |
| VS Code extension scaffold | ✅ shipped |
| GitHub public release (README, LICENSE, CI, templates) | ✅ shipped |
| 32 tests passing, golden test_all.q preserved | ✅ shipped |

## 🚧 Phase 2 — Type System + Modules (months 4–6)

- **Local variable declarations** inside function bodies (currently only `conf.qplc` globals)
- **Type inference** for `INT` / `REAL` / `BOOL` / `TIME`
- **Enums and structs** (`type Color = RED | GREEN | BLUE`)
- **Module system**: `import mymod`, `export fn foo`, `qplc.mod` files
- **`qplc fmt`** — code formatter (PEP 8 style for QPLC)
- **`qplc lint`** — static checker with autofix
- **Generic functions** with type parameters

## 🏭 Phase 3 — Industrial Simulator (months 7–9)

- **OPC UA server** in QPLC.Core (alongside Modbus TCP)
- **MQTT bridge** (publish variable changes to broker)
- **Multi-PLC simulation** with simulated network latency
- **FMU export** for co-simulation with Simulink
- **Watchdog timer** and fault injection
- **Soft-PLC I/O simulation** (encoders, PWM, analog filters)

## 📐 Phase 4 — IEC Complete (months 10–12)

- **FBD editor** in QPLC.Studio (function block diagram)
- **SFC editor** (sequential function chart)
- **PLCopen Motion blocks** (MC_MoveAbsolute, MC_MoveVelocity)
- **PLCopen Safety blocks** (SF_EmergencyStop, SF_SafeStop)
- **Import legacy LD** (PLCopen XML 1.0)
- **Web IDE** with WASM-compiled QPLC running entirely in the browser

## 🌐 Phase 5 — Ecosystem (month 12+)

- **Package manager**: `qplc` registry (think crates.io for PLC libraries)
- **Distribution**: NuGet (.NET libs), Scoop (Windows), Homebrew (macOS), apt (Debian/Ubuntu)
- **Documentation site** (VitePress) with tutorial, examples, deep dives
- **Stable 1.0** release with API stability promise
- **Commercial support tier** (optional, for industrial users who need SLAs)

---

Contributions welcome! Pick an item, open an issue to discuss, then submit a PR.
