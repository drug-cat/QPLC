using System;
using System.IO;

namespace QPLCExporter
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: QPLCExporter <config.qplc> <ladder.xml>");
                return;
            }

            string configPath = args[0];
            string ladderPath = args[1];

            Console.WriteLine("Reading config...");
            var config = ConfigParser.Parse(configPath);

            Console.WriteLine("Reading Ladder XML...");
            var networks = LadderXmlParser.Parse(ladderPath);

            Console.WriteLine($"Found {networks.Count} networks.");

            // Build the TIA project (currently only project creation)
            TiaProjectBuilder.Build(config, networks);
        }
    }
}