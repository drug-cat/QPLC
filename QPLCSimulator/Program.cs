using System;

namespace QPLCSimulator
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: QPLCSimulator <config.qplc> <output.xml>");
                return;
            }

            string configPath = args[0];
            string ladderPath = args[1];

            var networks = LadderXmlParser.Parse(ladderPath);
            var simulator = new LadderSimulator(networks);

            Console.WriteLine("QPLC Textual Simulator v0.2");
            Console.WriteLine("Commands:");
            Console.WriteLine("  set <var> <True|False>  - set boolean variable");
            Console.WriteLine("  set <var> <number>     - set numeric variable");
            Console.WriteLine("  tick <milliseconds>    - advance simulation time");
            Console.WriteLine("  run                    - run one scan cycle");
            Console.WriteLine("  show                   - show all variables and time");
            Console.WriteLine("  reset                  - reset all variables and time");
            Console.WriteLine("  exit                   - quit");
            Console.WriteLine();

            while (true)
            {
                Console.Write("> ");
                string? input = Console.ReadLine();
                if (string.IsNullOrWhiteSpace(input)) continue;

                string[] parts = input.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length == 0) continue;

                switch (parts[0].ToLower())
                {
                    case "set":
                        if (parts.Length < 3)
                        {
                            Console.WriteLine("Usage: set <var> <value>");
                            break;
                        }
                        string varName = parts[1];
                        string value = parts[2];
                        if (bool.TryParse(value, out bool boolVal))
                        {
                            simulator.SetBool(varName, boolVal);
                            Console.WriteLine($"{varName} = {boolVal}");
                        }
                        else if (double.TryParse(value, out double numVal))
                        {
                            simulator.SetNumeric(varName, numVal);
                            Console.WriteLine($"{varName} = {numVal}");
                        }
                        else
                        {
                            Console.WriteLine("Invalid value.");
                        }
                        break;

                    case "tick":
                        if (parts.Length < 2)
                        {
                            Console.WriteLine("Usage: tick <milliseconds>");
                            break;
                        }
                        if (double.TryParse(parts[1], out double ms))
                        {
                            simulator.AdvanceTime(ms);
                            Console.WriteLine($"Time advanced by {ms} ms. Current time: {simulator.CurrentTime} ms");
                        }
                        else
                        {
                            Console.WriteLine("Invalid time.");
                        }
                        break;

                    case "run":
                        simulator.RunScan();
                        Console.WriteLine("Scan completed.");
                        break;

                    case "show":
                        simulator.ShowVariables();
                        break;

                    case "reset":
                        simulator.Reset();
                        Console.WriteLine("Variables and time reset.");
                        break;

                    case "exit":
                        return;

                    default:
                        Console.WriteLine("Unknown command.");
                        break;
                }
            }
        }
    }
}