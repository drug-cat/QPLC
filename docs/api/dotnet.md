# .NET API — QPLC.Core

`QPLC.Core` is a .NET 8 class library shared by all simulators. It provides the configuration parser, ladder XML parser, the cycle-accurate ladder simulator, and the Modbus TCP server.

## Install

```bash
dotnet add package QPLC.Core
```

Requires .NET 8.0 or later.

## Overview

| Type | Purpose |
|------|---------|
| `ConfigParser` | Parses `conf.qplc` into a `Config` |
| `LadderXmlParser` | Parses the compiler's `output.xml` into `LadderNetwork`s |
| `LadderSimulator` | Executes ladder networks scan-by-scan |
| `ModbusServer` | Exposes a live simulation over Modbus TCP |

## Quick start

```csharp
using QPLC.Core;

// 1. Load configuration and ladder XML
var config = ConfigParser.Parse("conf.qplc");
var networks = LadderXmlParser.Parse("output.xml");

// 2. Create the simulator
var sim = new LadderSimulator(networks);

// 3. Set inputs and run a scan
sim.SetBool("start_button", true);
sim.RunScan();

// 4. Read outputs
var running = sim.BoolVars["motor_run"];
```

## ConfigParser

Parses the hardware, constants, IO, and simulator sections of `conf.qplc`:

```csharp
var config = ConfigParser.Parse("conf.qplc");

config.Hardware.Cpu;                 // "S7-1214C"
config.Hardware.Ip;                  // "192.168.0.1"
config.Constants["MAX_TEMP"];        // "80.0"
config.Io["motor_run"].Address;      // "Q0.0"
config.Io["motor_run"].Type;         // "BOOL"
config.Simulator.ModbusPort;         // 5020
```

- `IoMapping` — `Address`, `Type`, `ArrayLength`
- `HardwareConfig` — `Cpu`, `Ip`
- `SimulatorConfig` — `ModbusEnabled`, `ModbusPort`

## LadderXmlParser

```csharp
var networks = LadderXmlParser.Parse("output.xml");
```

Returns `List<LadderNetwork>`, each containing:

- `List<List<LadderElement>> Branches` — contact branches (rows of elements)
- `LadderElement? Coil` — coil output
- `LadderElement? Move` — numeric move/coil

`LadderElement` describes a single contact, coil, or function block via `Type`, `Address`, `ContactType`, `Op`, `Left`/`Right`, `Label`, `JumpType`/`Dest`, and `Input1..4`/`Preset`.

## LadderSimulator

```csharp
var sim = new LadderSimulator(networks);
```

### State

- `double CurrentTime` — elapsed simulation time (ms)
- `long ScanCount` — number of scans executed
- `IReadOnlyDictionary<string, bool> BoolVars` — boolean variables
- `IReadOnlyDictionary<string, double> NumVars` — numeric variables
- `int MaxStepsPerScan` (default 100,000) — safety cap for cross-network jumps

### Methods

```csharp
sim.SetBool("start_button", true);   // write a boolean input
sim.SetNumeric("count", 3.0);        // write a numeric variable
sim.RunScan();                       // execute one full scan cycle
sim.AdvanceTime(5000.0);             // advance the simulation clock (ms)
sim.Reset();                         // reset all state
sim.ShowVariables();                 // pretty-print all variables
```

### Timers, counters, edges

Timers (`TON`/`TOF`/`TP`), counters (`CTU`/`CTD`/`CTUD`), edge detection, and IEC math (`MIN`/`MAX`/`ABS`/`LIMIT`/`SEL`/`MUX`) are evaluated natively. Timer elapsed time (`ET`) progresses automatically as you call `AdvanceTime`.

### Time-travel trace

Every scan, the simulator records a `TraceSnapshot`. The last 1,000 snapshots are kept in a circular buffer:

```csharp
int n = sim.TraceCount;                      // snapshots available
var snap = sim.GetTraceSnapshot(fromEnd: 0); // most recent
var all  = sim.GetAllSnapshots();            // full history
```

The replay slider in QPLC.Studio reads these snapshots to let you step back through execution.

## ModbusServer

```csharp
using var modbus = new ModbusServer(sim, config, port: 5020);
modbus.Start();
// ... later ...
modbus.Stop();
```

- Pure C# implementation, **no external dependencies**
- Supports FC 1/2/3/5/6/15/16
- `CoilCount` and `HoldingRegisterCount` expose the mapped address count

Boolean variables map to coils; numerics map to holding registers. See the [Modbus guide](../guide/modbus) for the full protocol walkthrough.
