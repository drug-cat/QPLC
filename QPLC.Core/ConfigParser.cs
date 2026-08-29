using System;
using System.Collections.Generic;
using System.IO;

namespace QPLC.Core
{
    /// <summary>An IO mapping (hardware address, type, array length).</summary>
    public class IoMapping
    {
        public string Address { get; set; } = "";
        public string Type { get; set; } = "";
        public int ArrayLength { get; set; } = 1;
    }

    /// <summary>Configuration of the target hardware (CPU and IP).</summary>
    public class HardwareConfig
    {
        public string Cpu { get; set; } = "";
        public string Ip { get; set; } = "";
    }

    /// <summary>Complete program configuration: hardware, IO, constants, and simulator settings.</summary>
    public class Config
    {
        public HardwareConfig Hardware { get; set; } = new HardwareConfig();
        public Dictionary<string, IoMapping> Io { get; set; } = new Dictionary<string, IoMapping>();
        public Dictionary<string, string> Constants { get; set; } = new Dictionary<string, string>();
        public SimulatorConfig Simulator { get; set; } = new SimulatorConfig();
    }

    /// <summary>Settings for the [simulator] section (Modbus TCP and so on).</summary>
    public class SimulatorConfig
    {
        public bool ModbusEnabled { get; set; } = false;
        public int ModbusPort { get; set; } = 5020;
    }

    public static class ConfigParser
    {
        /// <summary>Reads a conf.qplc file and converts it to a Config.</summary>
        public static Config Parse(string filePath)
        {
            var config = new Config();
            string currentSection = "";

            foreach (var rawLine in File.ReadAllLines(filePath))
            {
                string line = rawLine.Trim();
                if (string.IsNullOrWhiteSpace(line) || line.StartsWith("#"))
                    continue;

                if (line.StartsWith("[") && line.EndsWith("]"))
                {
                    currentSection = line.Substring(1, line.Length - 2);
                    continue;
                }

                var eq = line.IndexOf('=');
                if (eq < 0) continue;

                string key = line.Substring(0, eq).Trim();
                string value = line.Substring(eq + 1).Trim();

                if (currentSection == "hardware")
                {
                    if (key == "cpu") config.Hardware.Cpu = value;
                    else if (key == "ip") config.Hardware.Ip = value;
                }
                else if (currentSection == "io")
                {
                    var colon = value.LastIndexOf(':');
                    if (colon < 0) continue;

                    string address = value.Substring(0, colon).Trim();
                    string typePart = value.Substring(colon + 1).Trim();

                    var mapping = new IoMapping { Address = address };

                    var bracket = typePart.IndexOf('[');
                    if (bracket >= 0)
                    {
                        var closeBracket = typePart.IndexOf(']', bracket);
                        if (closeBracket > bracket)
                        {
                            mapping.Type = typePart.Substring(0, bracket).Trim();
                            string lenStr = typePart.Substring(bracket + 1, closeBracket - bracket - 1);
                            if (int.TryParse(lenStr, out int n)) mapping.ArrayLength = n;
                        }
                    }
                    else
                    {
                        mapping.Type = typePart;
                    }

                    config.Io[key] = mapping;
                }
                else if (currentSection == "constants")
                {
                    config.Constants[key] = value;
                }
                else if (currentSection == "simulator")
                {
                    if (key == "modbus_enabled" && bool.TryParse(value, out bool me))
                        config.Simulator.ModbusEnabled = me;
                    else if (key == "modbus_port" && int.TryParse(value, out int mp))
                        config.Simulator.ModbusPort = mp;
                }
            }

            return config;
        }
    }
}
