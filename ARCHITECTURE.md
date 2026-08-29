# QPLC Architecture

## Goals

1. **Industrial quality** — correctness, deterministic codegen, predictable simulator behavior
2. **Cross-platform** — Windows, Linux, macOS at every layer
3. **Openness** — every artifact (Ladder XML, SCL, conf) is plain text and reviewable
4. **Composability** — third-party tools can consume our outputs and produce inputs we consume

## Pipeline

```
 ┌──────────┐    ┌────────┐    ┌──────────────┐    ┌─────────────┐
 │  .q      │───►│ Lexer  │───►│ Tokens       │───►│ Parser      │
 │  source  │    │ (C++)  │    │ (line, col)  │    │ (recursive  │
 │  + conf  │    │        │    │              │    │  descent)   │
 └──────────┘    └────────┘    └──────────────┘    └──────┬──────┘
                                                          │
                                                          ▼
                                                  ┌──────────────┐
                                                  │  AST         │
                                                  │  (typed)     │
                                                  └──────┬───────┘
                                                         │
                                                         ▼
                                                  ┌──────────────┐
                                                  │  Semantic    │
                                                  │  Analyzer    │
                                                  │  (errors)    │
                                                  └──────┬───────┘
                                                         │
                                ┌────────────────────────┼────────────────────────┐
                                ▼                                                 ▼
                       ┌────────────────┐                                 ┌──────────────┐
                       │  Ladder        │                                 │  SCL         │
                       │  Generator     │                                 │  Generator   │
                       │  (DNF-based)   │                                 │  (TIA Portal │
                       │                │                                 │   V19)       │
                       └────────┬───────┘                                 └──────┬───────┘
                                │                                                 │
                                ▼                                                 ▼
                       ┌────────────────┐                                 ┌──────────────┐
                       │  output.xml    │                                 │  output.scl  │
                       └────────┬───────┘                                 └──────────────┘
                                │
                ┌───────────────┼───────────────┐
                ▼               ▼               ▼
         ┌───────────┐  ┌──────────────┐  ┌────────────┐
         │ Console   │  │ WPF Studio   │  │ Avalonia   │
         │ Simulator │  │ (legacy)     │  │ Studio     │
         └─────┬─────┘  └──────┬───────┘  └─────┬──────┘
               │               │                │
               └───────────────┴────────────────┘
                               │
                               ▼
                       ┌──────────────┐
                       │  QPLC.Core   │
                       │  (class lib) │
                       └──────┬───────┘
                              │
                  ┌───────────┼───────────┐
                  ▼           ▼           ▼
           ┌──────────┐ ┌──────────┐ ┌──────────┐
           │ Parser   │ │Simulator │ │ Modbus   │
           │          │ │ + Trace  │ │ TCP      │
           └──────────┘ └──────────┘ └──────────┘
```

## C++ Compiler (`qplc`)

| Component | File | Role |
|-----------|------|------|
| Lexer | `src/lexer/lexer.cpp` | Indentation-aware tokenizer (INDENT/DEDENT), produces `Token` stream with line/col |
| Parser | `src/parser/parser.cpp` | Recursive descent, builds typed AST |
| AST | `src/ast/ast.h` | Immutable tree with `unique_ptr` ownership |
| Semantic | `src/semantic/semantic_analyzer.cpp` | Scope checking, undefined variable detection, function arity, constant read-only enforcement |
| Ladder codegen | `src/codegen/ladder_generator.cpp` | Boolean expressions → DNF → contacts/coils; numeric → move rungs; timers/counters as first-class |
| SCL codegen | `src/codegen/scl_generator.cpp` | 1-to-1 mapping with IEC function names (`TON`, `LIMIT`, `MUX`, …) |
| LSP | `src/lsp/lsp_server.cpp` | JSON-RPC over stdio, publishes diagnostics on text change |

### DNF codegen (why DNF?)

PLC ladder diagrams are fundamentally 2D: each rung is an OR of AND-terms. Any boolean expression `f(x₁, …, xₙ)` can be written in **disjunctive normal form** (a sum of products) which maps directly to ladder contacts. The compiler:

1. Walks the expression tree
2. Distributes `∨` over `∧` to get DNF
3. Emits one rung per DNF term
4. Shares contacts when possible

For `a and b`, that's one rung with two NO contacts in series.
For `(a and b) or c`, that's two rungs in parallel.

Ternary `a if c else b` compiles to `c ∧ a ∨ ¬c ∧ b` = two DNF terms, simplified to `c ∧ a` joined with `¬c ∧ b` (the simplifier omits the trivial `a` term when it appears alone).

## C# Runtime (`QPLC.Core`)

| Component | File | Role |
|-----------|------|------|
| ConfigParser | `ConfigParser.cs` | INI-like `conf.qplc` → `Config` object |
| LadderXmlParser | `LadderXmlParser.cs` | `output.xml` → list of `LadderNetwork` |
| LadderSimulator | `LadderSimulator.cs` | Execute ladder: timers, counters, edges, IEC math, trace |
| ModbusServer | `ModbusServer.cs` | Modbus TCP server (FC 1/2/3/5/6/15/16) |
| Trace | `LadderSimulator.TraceSnapshot` | 1000-scan circular buffer |

All three simulators (Console, WPF, Avalonia) reference `QPLC.Core` — there's no code duplication.

## Why Ladder XML, not a binary format?

- **Diffable** in git
- **Human-readable** for debugging
- **Parseable** by anyone (no proprietary toolchain needed)
- **Forward-compatible** — adding a new element is non-breaking (older parsers ignore unknown tags)

## Why C++20 for the compiler, .NET 8 for the runtime?

- **Compiler**: needs fast cold-start, low memory, single-file distribution, no runtime. C++ with static linking gives us a 3MB self-contained `qplc.exe`.
- **Runtime**: needs cross-platform GUI (Avalonia), rich standard library for HTTP/serial/etc., Modbus libs. .NET 8 is the right tool.

## Extension points

- New IEC functions: add to `src/common/builtins.h` + ladder codegen + SCL codegen + simulator
- New simulator backends: reference `QPLC.Core`, implement a thin UI layer
- New code generators: add a new file in `src/codegen/`, hook into `main.cpp`
- New front-ends (e.g. ST, SFC): add a parser, share AST, reuse codegen
