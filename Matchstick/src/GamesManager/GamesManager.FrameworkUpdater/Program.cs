using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
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
            var commandLineRepeater = new CommandLineRepeater(Environment.GetCommandLineArgs());

            if (FileSystem.Current.FileExists(Resources.GamesManagerNewExecutable))
            {
                //continunally tries to delete the gamesmanager exe and replace it with the newly downloaded version
                while (true)
                {
                    try
                    {
                        FileSystem.Current.GetFile(Resources.GamesManagerExecutable).DeleteWithTimeout();
                        FileSystem.Current.GetFile(Resources.GamesManagerNewExecutable).MoveTo(Resources.GamesManagerExecutable);
                        break;
                    }
                    catch (Exception)
                    {
                    }
                }
            }

            Process.Start(Resources.GamesManagerExecutable, commandLineRepeater.ToString());
        }
    }
}