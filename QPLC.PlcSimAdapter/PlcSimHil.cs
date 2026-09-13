using QPLC.Core;

namespace QPLC.PlcSimAdapter;

/// <summary>
/// Bridges a QPLC ladder simulation to a Siemens PLCSIM Advanced virtual
/// controller. On every scan:
///   1. reads mapped I-point tags from PLCSIM → feeds QPLC inputs,
///   2. runs one QPLC scan,
///   3. writes QPLC outputs back to PLCSIM Q-point tags.
/// The I/O mapping comes straight from conf.qplc ([io] section), so the
/// virtual PLC behaves like the real hardware described by the config.
/// </summary>
public sealed class PlcSimHil : IDisposable
{
    private readonly Config _config;
    private readonly LadderSimulator _sim;
    private readonly List<(string name, string tag, string type)> _inputs;
    private readonly List<(string name, string tag, string type)> _outputs;

    public PlcSimHil(Config config, LadderSimulator sim)
    {
        _config = config;
        _sim = sim;

        _inputs = new();
        _outputs = new();
        foreach (var (name, mapping) in config.Io)
        {
            string area = mapping.Address[0] switch
            {
                'I' => "I",
                'Q' => "Q",
                'M' => "M",
                _ => "M",
            };
            var entry = (name, mapping.Address, mapping.Type);
            if (area == "I") _inputs.Add(entry);
            else if (area == "Q" || area == "M") _outputs.Add(entry);
        }
    }

    public int Connect(string instanceName)
        => PlcSimBridge.qplc_plcsim_connect_instance(instanceName);

    public int Run(int timeoutMs = 1000)
        => PlcSimBridge.qplc_plcsim_run(timeoutMs);

    public int Stop(int timeoutMs = 1000)
        => PlcSimBridge.qplc_plcsim_stop(timeoutMs);

    /// <summary>Executes one synchronized scan: PLCSIM → QPLC → PLCSIM.</summary>
    public void RunOneScan()
    {
        // 1. Inputs: PLCSIM I-points → QPLC sim variables
        foreach (var (name, tag, type) in _inputs)
        {
            switch (type)
            {
                case "BOOL":
                    if (PlcSimBridge.qplc_plcsim_read_bool(tag, out int b) == PlcSimBridge.OK)
                        _sim.SetBool(name, b != 0);
                    break;
                case "INT":
                case "WORD":
                case "DINT":
                    if (PlcSimBridge.qplc_plcsim_read_int16(tag, out short s) == PlcSimBridge.OK)
                        _sim.SetNumeric(name, s);
                    break;
                case "REAL":
                case "TIME":
                    if (PlcSimBridge.qplc_plcsim_read_real(tag, out float f) == PlcSimBridge.OK)
                        _sim.SetNumeric(name, f);
                    break;
            }
        }

        // 2. Run the QPLC ladder scan
        _sim.RunScan();

        // 3. Outputs: QPLC sim variables → PLCSIM Q/M-points
        foreach (var (name, tag, type) in _outputs)
        {
            switch (type)
            {
                case "BOOL":
                    if (_sim.BoolVars.TryGetValue(name, out bool b))
                        PlcSimBridge.qplc_plcsim_write_bool(tag, b ? 1 : 0);
                    break;
                case "INT":
                case "WORD":
                case "DINT":
                    if (_sim.NumVars.TryGetValue(name, out double d0))
                        PlcSimBridge.qplc_plcsim_write_int16(tag, (short)d0);
                    break;
                case "REAL":
                case "TIME":
                    if (_sim.NumVars.TryGetValue(name, out double d1))
                        PlcSimBridge.qplc_plcsim_write_real(tag, (float)d1);
                    break;
            }
        }
    }

    public void Dispose()
    {
        Stop(500);
        PlcSimBridge.qplc_plcsim_shutdown();
    }
}