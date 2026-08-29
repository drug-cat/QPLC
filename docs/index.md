---
layout: home

hero:
  name: QPLC
  text: Industrial Ladder Logic DSL
  tagline: A modern Python-like language that compiles to Ladder Logic and SCL for Siemens S7-1200 PLCs — with a high-fidelity simulator, Modbus TCP server, and time-travel debugger.
  actions:
    - theme: brand
      text: Get Started
      link: /guide/installation
    - theme: alt
      text: Language Reference
      link: /guide/language
    - theme: alt
      text: View on GitHub
      link: https://github.com/YOUR_USERNAME/QPLC

features:
  - icon: 🐍
    title: Python-like Syntax
    details: Indentation-based control flow with if/while/for, def, return, and Python-style ternary `a if cond else b`.
  - icon: ⚡
    title: Modern C++20 Compiler
    details: Recursive descent parser, semantic analyzer, and DNF-based ladder codegen that emits clean, importable XML.
  - icon: 🏭
    title: Industrial I/O
    details: IEC timers (TON/TOF/TP), counters (CTU/CTD/CTUD), and edge detection built in.
  - icon: 🪜
    title: Ladder + SCL Export
    details: Ladder XML for the simulator and SCL directly importable into TIA Portal V19.
  - icon: 🔌
    title: Modbus TCP Server
    details: Connect SCADA/HMI to a live simulation over Modbus TCP (FC 1/2/3/5/6/15/16).
  - icon: ⏪
    title: Time-Travel Debug
    details: A 1000-scan circular snapshot buffer with replay for diagnosing elusive control bugs.
---

## Quick Start

```bash
# Build the compiler
cmake -S . -B build -G Ninja
cmake --build build

# Compile a program to Ladder XML
./build/qplc examples/conf.qplc examples/test_all.q -o output.xml
```

Then run the simulator:

```bash
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc output.xml
```

::: tip
QPLC is open source under the Apache-2.0 license. Star it on [GitHub](https://github.com/YOUR_USERNAME/QPLC) to stay updated.
:::

<!-- Community badges -->
<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg" alt="License"></a>
  <a href="src"><img src="https://img.shields.io/badge/C++-20-00599C.svg" alt="C++20"></a>
  <a href="QPLC.Core"><img src="https://img.shields.io/badge/.NET-8.0-512BD4.svg" alt=".NET 8"></a>
  <a href="tests"><img src="https://img.shields.io/badge/tests-32%2F32-brightgreen.svg" alt="Tests"></a>
</p>
