# Modbus TCP Server

QPLC.Core ships a pure C# Modbus TCP server with **no external dependencies**. It exposes the live simulation to any SCADA, HMI, or MQTT gateway over the network.

The server supports the standard function codes:

| Function code | Name | Direction |
|---------------|------|-----------|
| FC 1 | Read Coils | PLC → client |
| FC 2 | Read Discrete Inputs | PLC → client |
| FC 3 | Read Holding Registers | PLC → client |
| FC 5 | Write Single Coil | client → PLC |
| FC 6 | Write Single Register | client → PLC |
| FC 15 | Write Multiple Coils | client → PLC |
| FC 16 | Write Multiple Registers | client → PLC |

## Configuration

Enable the server and choose a port in `conf.qplc`:

```ini
[simulator]
modbus_port = 5020
```

When the simulator starts, it listens on the configured port (default `5020`).

## By default in the console simulator

Start the console simulator with Modbus enabled:

```bash
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc output.xml
```

Then connect any Modbus master — e.g. with `mbpoll`:

```bash
mbpoll 127.0.0.1 -p 5020 -t 0 -r 0 -c 8      # read 8 coils
mbpoll 127.0.0.1 -p 5020 -t 3 -r 0 -c 8      # read 8 holding registers
```

## Address mapping

Boolean I/O (`BOOL`) map to **coils / discrete inputs**; numerics (`INT`, `REAL`, `TIME`) map to **holding registers**. The mapping order follows the `[io]` table, with `I*` inputs exposed as read-only and `Q*`/`M*` as read-write coils/registers.

## Using QPLC.Core directly

```csharp
using QPLC.Core;

var config = ConfigParser.ParseFile("conf.qplc");
var sim = new LadderSimulator(config);
sim.Load("output.xml");

using var modbus = new ModbusServer(sim, config, port: 5020);
modbus.Start();

Console.WriteLine("Modbus listening on port 5020. Press Enter to stop.");
Console.ReadLine();
```

This is a real Hardware-in-the-Loop (HIL) setup: a physical SCADA/HMI can read live scan data and write setpoints while the simulator runs.
