# QPLC.Core

A cross-platform PLC ladder logic simulator with Modbus TCP, time-travel debugging, and IEC 61131-3 support.

## Installation

```bash
dotnet add package QPLC.Core
```

## Quick Start

```csharp
using QPLC.Core;

// Parse conf.qplc
var config = ConfigParser.Parse("conf.qplc");

// Parse compiled ladder output.xml
var networks = LadderXmlParser.Parse("output.xml");

// Build simulator
using var sim = new LadderSimulator(config, networks);

// Run one scan
sim.Tick();

// Read variables
bool motorRun = sim.GetBool("motor_run");
double speed = sim.GetNumeric("speed");

// Modbus TCP server
var modbus = new ModbusServer(sim, port: 502);
modbus.Start();
```

## Features

- **Ladder Logic Simulation** — runs compiled QPLC ladder networks
- **Time-Travel Debugging** — circular buffer of 1000 scan snapshots with `TraceReplay`
- **Modbus TCP Server** — pure C# implementation (no external dependencies), FC 1/2/3/5/6/15/16
- **IEC Math Functions** — `MIN`, `MAX`, `ABS`, `LIMIT`, `SEL`, `MUX` in expression evaluator
- **Timers & Counters** — TON, TOF, TP, CTU, CTD with elapsed/remaining time queries
- **Cross-Platform** — .NET 8 (Windows, Linux, macOS)

## Documentation

Full documentation: https://github.com/YOUR_USERNAME/QPLC

## License

Apache-2.0
