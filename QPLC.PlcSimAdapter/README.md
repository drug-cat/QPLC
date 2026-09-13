# QPLC.PlcSimAdapter — Hardware-in-the-Loop with Siemens PLCSIM Advanced

Bridges a **QPLC ladder simulation** to a **Siemens PLCSIM Advanced** virtual
controller (S7-1500) running inside TIA Portal. On every synchronized scan:

1. reads the mapped `I*-point` tags from the virtual PLC,
2. runs one QPLC scan,
3. writes the QPLC outputs back to the `Q*/M*` tags.

Because the `[io]` mapping comes straight from `conf.qplc`, the virtual PLC
behaves exactly like the physical hardware described by your QPLC config —
a true Hardware-in-the-Loop (HIL) setup.

## Requirements

- Siemens **PLCSIM Advanced** (any installed version; API 1.0–6.0 auto-detected)
  — installed at `C:\Program Files (x86)\Common Files\Siemens\PLCSIMADV`
- A virtual controller instance created in **PLCSIM Advanced / TIA Portal**
- .NET 8 SDK (to run the C# driver)

## Build

```bash
# 1. Build the native C bridge (auto-detects your PLCSIM Advanced API version)
cmake -S . -B build -G Ninja
cmake --build build

# 2. Build the .NET driver
dotnet build QPLC.PlcSimAdapter/QPLC.PlcSimAdapter.csproj
```

The build produces:

- `build/libqplc_plcsim_bridge.dll` — the native P/Invoke bridge
- `QPLC.PlcSimAdapter/bin/Debug/net8.0/QPLC.PlcSimAdapter.dll` — the driver

If PLCSIM Advanced is not installed, CMake simply skips the bridge target.

## Run

```bash
# Compile a QPLC program to ladder XML
./build/qplc examples/plcsim_conf.qplc examples/plcsim_demo.q -o plcsim_out.xml

# Start the virtual PLC in PLCSIM Advanced, then:
dotnet QPLC.PlcSimAdapter/bin/Debug/net8.0/QPLC.PlcSimAdapter.dll \
  examples/plcsim_conf.qplc plcsim_out.xml MyInstance 10
```

- `<instanceName>` is the virtual controller name you see in PLCSIM Advanced.
- The last argument (optional) is scans/second; match it to the vPLC cycle
  time (default 10).

Set `start_btn` to `True` in the vPLC and watch `motor_run` / `speed_ref`
follow — the ladder executes live, scan by scan.

## I/O mapping

| conf.qplc area | PLCSIM tag | Direction |
|----------------|-----------|-----------|
| `I*:BOOL`      | `I0.0`, …  | vPLC → QPLC |
| `IW*:INT/REAL` | `IW64`, …  | vPLC → QPLC |
| `Q*:BOOL`      | `Q0.0`, …  | QPLC → vPLC |
| `MW/QW*:INT/REAL` | `MW10`, … | QPLC → vPLC |