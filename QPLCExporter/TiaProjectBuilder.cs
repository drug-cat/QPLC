using System;
using System.Collections.Generic;
using Siemens.Engineering;

namespace QPLCExporter
{
    public static class TiaProjectBuilder
    {
        public static void Build(Config config, List<LadderNetwork> networks)
        {
            // Project path and name
            string projectDirectory = Path.GetFullPath(".");
            string projectName = "QPLC_Project";

            using (TiaPortal portal = new TiaPortal())
            {
                // Create a new project with the correct method
                Project project = portal.Projects.Create(
                    new DirectoryInfo(projectDirectory),
                    projectName);

                Console.WriteLine($"Project '{project.Name}' created successfully.");
                Console.WriteLine($"CPU from config: {config.Hardware.Cpu}");

                // TODO: adding the CPU, tags, and ladder networks requires Openness SW/HW to be installed
                Console.WriteLine("Device, tags, and ladder import not yet available (missing SW/HW Openness components).");

                project.Save();
                project.Close();
            }

            Console.WriteLine("Done.");
        }
    }
}