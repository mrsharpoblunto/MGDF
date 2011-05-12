using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.Tests.Common.Mocks;

namespace MGDF.GamesManager.Service.Console
{
    class ConsoleLogger: ILogger
    {
        public void Write(LogInfoLevel level, string message)
        {
            System.Console.WriteLine(level+": "+message);
        }

        public void Write(Exception ex, string message)
        {
            System.Console.WriteLine("Error: " + message);
            System.Console.WriteLine("\tDetail: " + ex);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length==1 && args[0] == "-usemocks")
            {
                Logger.Current = new ConsoleLogger();
                FileSystem.Current = new MockFileSystem();
                EnvironmentSettings.Current = new MockEnvironmentSettings();
                Registry.Current = new MockRegistry();
                GameExplorer.Current = new MockGameExplorer();
                IconManager.Current = new MockIconManager();
                ProcessManager.Current = new MockProcessManager();
                ServiceManager.Current = new MockServiceManager();
                ShortcutManager.Current = new MockShortcutManager();
                ProcessManager.Current = new MockProcessManager();
                ArchiveFactory.Current = new ArchiveFactory();
                TimeService.Current = new TimeService();
                EntityFactory.Current = new EntityFactory();
                IdentityGenerator.Current = new IdentityGenerator();
                HttpRequestManager.Current = new HttpRequestManager();

                ((MockRegistry)Registry.Current).AddFakeKey(BaseRegistryKey.LocalMachine, new MockRegistryKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));

                MockDirectory volumeRoot = ((MockFileSystem)FileSystem.Current).AddVolumeRoot("C:");
                volumeRoot.AddDirectory("temp");
                MockDirectory documentsDir = volumeRoot.AddDirectory("Documents and Settings");
                documentsDir.AddDirectory("All Users").AddDirectory("Application Data");
                MockDirectory userDir = documentsDir.AddDirectory("user");
                userDir.AddDirectory("Local Settings").AddDirectory("Application Data");
                userDir.AddDirectory("desktop");
                userDir.AddDirectory("start menu");
                volumeRoot.AddDirectory("Documents and Settings").AddDirectory("All Users").AddDirectory("Application Data");

                MockDirectory schemasDir = volumeRoot.AddDirectory("program files").AddDirectory("MGDF").AddDirectory("schemas");
                schemasDir.AddFile("game.xsd", ReadTextFile("Schemas\\game.xsd"));
                schemasDir.AddFile("gameState.xsd", ReadTextFile("Schemas\\gameState.xsd"));
                schemasDir.AddFile("playlist.xsd", ReadTextFile("Schemas\\playlist.xsd"));
                schemasDir.AddFile("preferences.xsd", ReadTextFile("Schemas\\preferences.xsd"));
                schemasDir.AddFile("statistics.xsd", ReadTextFile("Schemas\\statistics.xsd"));
                schemasDir.AddFile("update.xsd", ReadTextFile("Schemas\\update.xsd"));

                Constants.CreateRequiredCommonDirectories();
            }

            var container = new GamesManagerServiceContainer(args.Length==0);

            System.Console.WriteLine("Starting...");
            container.OnStart(new string[0]);
            System.Console.WriteLine("Started");
            System.Console.WriteLine();
            System.Console.WriteLine("Press H to hibernate/resume, S to stop/start, and Q to shut down.");

            bool running = true;
            bool started = true;
            bool hibernating = false;
            while (running)
            {
                var key = System.Console.ReadKey(true); 
                switch (key.Key)
                {
                    case ConsoleKey.Q:
                        running = false;
                        break;
                    case ConsoleKey.H:
                        if (started)
                        {
                            if (!hibernating)
                            {
                                System.Console.WriteLine("Hibernating...");
                                container.OnPowerEvent(PowerBroadcastStatus.Suspend);
                                System.Console.WriteLine("Hibernated");
                                hibernating = true;
                            }
                            else
                            {
                                System.Console.WriteLine("Resuming...");
                                container.OnPowerEvent(PowerBroadcastStatus.ResumeSuspend);
                                System.Console.WriteLine("Resumed");
                                hibernating = false;
                            }
                        }
                        break;
                    case ConsoleKey.S:
                        if (!hibernating)
                        {
                            if (!started)
                            {
                                System.Console.WriteLine("Starting...");
                                container.OnStart(new string[0]);
                                System.Console.WriteLine("Started");
                                started = true;
                            }
                            else
                            {
                                System.Console.WriteLine("Stopping...");
                                container.OnStop();
                                System.Console.WriteLine("Stopped");
                                started = false;
                            }
                        }
                        break;
                }
            }

            System.Console.WriteLine("Shutting down...");
            container.OnShutdown();
            System.Console.WriteLine("Shut down");
        }

        private static string ReadTextFile(string filename)
        {
            using (FileStream stream = new FileStream(filename, FileMode.Open, FileAccess.Read))
            {
                using (TextReader reader = new StreamReader(stream))
                {
                    return reader.ReadToEnd();
                }
            }
        }
    }
}