using System;
using System.Collections.Generic;
using Siemens.Engineering;

namespace QPLCExporter
{
    public static class TiaProjectBuilder
    {
        public static void Build(Config config, List<LadderNetwork> networks)
        {
            // مسیر و نام پروژه
            string projectDirectory = Path.GetFullPath(".");
            string projectName = "QPLC_Project";

            using (TiaPortal portal = new TiaPortal())
            {
                // ایجاد پروژه جدید با متد صحیح
                Project project = portal.Projects.Create(
                    new DirectoryInfo(projectDirectory),
                    projectName);

                Console.WriteLine($"Project '{project.Name}' created successfully.");
                Console.WriteLine($"CPU from config: {config.Hardware.Cpu}");

                // TODO: افزودن CPU، تگ‌ها و شبکه‌های Ladder نیاز به نصب Openness SW/HW دارد
                Console.WriteLine("Device, tags, and ladder import not yet available (missing SW/HW Openness components).");

                project.Save();
                project.Close();
            }

            Console.WriteLine("Done.");
        }
    }
}