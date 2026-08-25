using System;
using System.Collections.Generic;
using System.IO;

namespace QPLCExporter
{
    public class IoMapping
    {
        public string Address { get; set; }
        public string Type { get; set; }
        public int ArrayLength { get; set; } = 1;
    }

    public class HardwareConfig
    {
        public string Cpu { get; set; }
        public string Ip { get; set; }
    }

    public class Config
    {
        public HardwareConfig Hardware { get; set; } = new HardwareConfig();
        public Dictionary<string, IoMapping> Io { get; set; } = new Dictionary<string, IoMapping>();
    }

    public static class ConfigParser
    {
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
                            mapping.ArrayLength = int.Parse(lenStr);
                        }
                    }
                    else
                    {
                        mapping.Type = typePart;
                    }

                    config.Io[key] = mapping;
                }
            }

            return config;
        }
    }
}