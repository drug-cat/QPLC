using System;
using System.IO;
using QPLC.Core;

namespace QPLCSimulator
{
    public class Program
    {
        public static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: QPLCSimulator <config.qplc> <output.xml> [--modbus]");
                return;
            }

            string confPath = args[0];
            string xmlPath = args[1];
            bool enableModbus = args.Length > 2 && args[2] == "--modbus";

            var config = ConfigParser.Parse(confPath);
            var networks = LadderXmlParser.Parse(xmlPath);
            var sim = new LadderSimulator(networks);

            if (enableModbus || config.Simulator.ModbusEnabled)
            {
                int port = config.Simulator.ModbusPort > 0 ? config.Simulator.ModbusPort : 5020;
                using var modbus = new ModbusServer(sim, config, port);
                modbus.Start();
                Console.WriteLine($"Modbus TCP server started on 127.0.0.1:{port} " +
                                  $"({modbus.CoilCount} coils, {modbus.HoldingRegisterCount} holding registers)");
            }

            Console.WriteLine("QPLC Simulator REPL — commands: set, tick, run, show, reset, trace, help, exit");
            Console.WriteLine();

            while (true)
            {
                Console.Write("> ");
                var line = Console.ReadLine();
                if (line == null) break;
                line = line.Trim();
                if (line.Length == 0) continue;

                var parts = line.Split(' ', 3);
                var cmd = parts[0].ToLowerInvariant();

                try
                {
                    switch (cmd)
                    {
                        case "exit":
                        case "quit":
                            return;

                        case "set":
                            if (parts.Length < 3) { Console.WriteLine("usage: set <name> <True|False|num>"); break; }
                            {
                                string n = parts[1];
                                string v = parts[2];
                                if (bool.TryParse(v, out bool b)) sim.SetBool(n, b);
                                else if (double.TryParse(v, out double d)) sim.SetNumeric(n, d);
                                else Console.WriteLine($"cannot parse '{v}'");
                            }
                            break;

                        case "tick":
                            if (parts.Length < 2 || !double.TryParse(parts[1], out double ms))
                            { Console.WriteLine("usage: tick <ms>"); break; }
                            sim.AdvanceTime(ms);
                            Console.WriteLine($"time = {sim.CurrentTime} ms");
                            break;

                        case "run":
                            sim.RunScan();
                            Console.WriteLine($"scan {sim.ScanCount} complete (t={sim.CurrentTime} ms)");
                            if (!string.IsNullOrEmpty(sim.LastWarning))
                                Console.WriteLine($"WARNING: {sim.LastWarning}");
                            break;

                        case "show":
                            if (parts.Length >= 2) ShowOne(sim, parts[1]);
                            else sim.ShowVariables();
                            break;

                        case "reset":
                            sim.Reset();
                            Console.WriteLine("reset complete");
                            break;

                        case "trace":
                            Console.WriteLine($"trace buffer: {sim.TraceCount} snapshots (max 1000)");
                            if (parts.Length >= 2 && int.TryParse(parts[1], out int idx))
                            {
                                var snap = sim.GetTraceSnapshot(idx);
                                if (snap == null) { Console.WriteLine("snapshot out of range"); break; }
                                Console.WriteLine($"snapshot #{idx} from end: scan={snap.ScanNumber} t={snap.Time} ms");
                                foreach (var kv in snap.BoolState.OrderBy(k => k.Key))
                                    Console.WriteLine($"  {kv.Key} = {kv.Value}");
                                foreach (var kv in snap.NumState.OrderBy(k => k.Key))
                                    Console.WriteLine($"  {kv.Key} = {kv.Value}");
                            }
                            break;

                        case "help":
                            Console.WriteLine("Commands:");
                            Console.WriteLine("  set <name> <True|False|num>  set variable");
                            Console.WriteLine("  tick <ms>                    advance simulation time");
                            Console.WriteLine("  run                          run one scan");
                            Console.WriteLine("  show [name]                  show all variables or one");
                            Console.WriteLine("  trace [n]                    list trace snapshots (n from end)");
                            Console.WriteLine("  reset                        reset simulator");
                            Console.WriteLine("  exit                         quit");
                            break;

                        default:
                            Console.WriteLine($"unknown command: {cmd} (try 'help')");
                            break;
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"error: {ex.Message}");
                }
            }
        }

        private static void ShowOne(LadderSimulator sim, string name)
        {
            if (sim.BoolVars.TryGetValue(name, out bool b))
                Console.WriteLine($"{name} = {b}");
            else if (sim.NumVars.TryGetValue(name, out double n))
                Console.WriteLine($"{name} = {n}");
            else
                Console.WriteLine($"variable '{name}' not found");
        }
    }
}
