using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.FrameworkUpdater
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            TimeService.Current = new TimeService();
            EnvironmentSettings.Current = new EnvironmentSettings();
            FileSystem.Current = new FileSystem();
            CommandLineRepeater commandLine = new CommandLineRepeater(Environment.GetCommandLineArgs());

            string updateFile = string.Empty;
            if (FrameworkUpdateChecker.RequiresUpdate(out updateFile))
            {
                Application.Run(new UpdatingForm(commandLine, updateFile));
            }
            else
            {
                Process.Start(Constants.GamesManagerExecutable, commandLine.ToString());
            }
        }
    }
}