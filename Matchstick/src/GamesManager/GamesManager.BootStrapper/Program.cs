using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.BootStrapper
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            TimeService.Current = new TimeService();
            EnvironmentSettings.Current = new EnvironmentSettings();
            FileSystem.Current = new FileSystem();
            CommandLineRepeater commandLine = new CommandLineRepeater(args);

            string updateFile;
            if (FrameworkUpdateChecker.RequiresUpdate(out updateFile))
            {
                Process.Start(Constants.CurrentFrameworkUpdaterExecutable, commandLine.ToString());
            }
            else
            {
                Process.Start(Constants.GamesManagerExecutable, commandLine.ToString());     
            }
        }
    }
}