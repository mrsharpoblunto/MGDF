using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Presenters;
using MGDF.GamesManager.MVP.Views.Impl;

namespace MGDF.GamesManager
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static int Main()
        {
            TimeService.Current = new TimeService();
            EnvironmentSettings.Current = new EnvironmentSettings();
            FileSystem.Current = new FileSystem();
            ArchiveFactory.Current = new ArchiveFactory();
            HttpRequestManager.Current = new HttpRequestManager();
            ProcessManager.Current = new ProcessManager();
            Registry.Current = new Registry();
            ShortcutManager.Current = new ShortcutManager();
            Config.Current = new Config();

            ViewImpl.RegisterViews();

            try
            {
                return RunMain();
            }
            catch (System.Security.SecurityException)
            {
                ViewFactory.Current.CreateView<IMessage>().Show("A Security exception occurred, This could be because you are running the GamesManager from a network share. If this is the case please try running it from a local folder.", "Security Exception");
            }
            catch (Exception ex)
            {
                ShowUnhandledError(ex);
            }
            return -1;
        }

        static int RunMain()
        {
            var commandLine = new CommandLineParser(Environment.GetCommandLineArgs());

            Application.ThreadException += Application_ThreadException;
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            Game game = new Game(FileSystem.Combine(Resources.GameBaseDir,Resources.GameConfig));
            if (game.ErrorCollection.Count>0)
            {
                ViewFactory.Current.CreateView<IMessage>().Show("Game configuration invalid " + game.ErrorCollection[0], "Game configuration invalid");
                return -1;
            }
            Game.Current = game;

            Resources.InitUserDirectory(Game.Current.Uid, commandLine[Resources.GamesManagerArguments.UserDirOverrideArgument]!=null);
            Logger.Current = new Logger(Path.Combine(Resources.GameUserDir, "GamesManagerLog.txt"));


            if (commandLine[Resources.GamesManagerArguments.RegisterArgument] != null)
            {
                if (!UACControl.IsAdmin())
                {
                    ViewFactory.Current.CreateView<IMessage>().Show("Registering requires administrator access", "Administrator accesss required");
                    return -1;
                }

                var installer = new GameRegistrar(true, Game.Current);
                return installer.Start()==LongRunningTaskResult.Completed ? 0: -1;
            }
            else if (commandLine[Resources.GamesManagerArguments.DeregisterArgument] != null)
            {
                if (!UACControl.IsAdmin())
                {
                    ViewFactory.Current.CreateView<IMessage>().Show("Deregistering requires administrator access", "Administrator accesss required");
                    return -1;
                }

                var uninstaller = new GameRegistrar(false, Game.Current);
                return uninstaller.Start() == LongRunningTaskResult.Completed ? 0 : -1;
            }
            else if (commandLine[Resources.GamesManagerArguments.UpdateFrameworkArgument] != null || commandLine[Resources.GamesManagerArguments.UpdateGameArgument] != null)
            {
                if (commandLine[Resources.GamesManagerArguments.UpdateFrameworkArgument] != null &&
                    commandLine[Resources.GamesManagerArguments.FrameworkUpdateHashArgument]==null)
                {
                    ViewFactory.Current.CreateView<IMessage>().Show("Framework update MD5 hash argument missing", "Missing argument");
                    return -1;
                }

                if (commandLine[Resources.GamesManagerArguments.UpdateGameArgument] != null &&
                    commandLine[Resources.GamesManagerArguments.GameUpdateHashArgument] == null)
                {
                    ViewFactory.Current.CreateView<IMessage>().Show("Game update MD5 hash argument missing", "Missing argument");
                    return -1;
                }

                if (!UACControl.IsAdmin())
                {
                    ViewFactory.Current.CreateView<IMessage>().Show("Updating requires administrator access", "Administrator accesss required");
                    return -1;
                }

                var presenter = new UpdateGamePresenter(
                    commandLine[Resources.GamesManagerArguments.UpdateGameArgument],
                    commandLine[Resources.GamesManagerArguments.GameUpdateHashArgument],
                    commandLine[Resources.GamesManagerArguments.UpdateFrameworkArgument], 
                    commandLine[Resources.GamesManagerArguments.FrameworkUpdateHashArgument]);
                presenter.ShowView();
                Application.Run(presenter.View as Form);

                Process.Start(Resources.FrameworkUpdaterExecutable, Resources.GamesManagerBootArguments(string.Empty,string.Empty,string.Empty,string.Empty));
            }
            else
            {
                var presenter = new LaunchGamePresenter(commandLine[Resources.GamesManagerArguments.NoUpdateCheckArgument] == null);
                presenter.ShowView();
                Application.Run(presenter.View as Form);
            }

            return 0;
        }

        public static void Application_ThreadException(object sender, ThreadExceptionEventArgs e)
        {
            ShowUnhandledError(e.Exception);
        }

        public static void ShowUnhandledError(Exception e)
        {
            try
            {
                IView latestView = ViewFactory.Current.LatestView;
                if (Logger.Current!=null) Logger.Current.Write(e, "Unexpected error in GamesManager");
                var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", e.ToString());
                controller.View.Closed += (s, args) => Application.Exit();
                controller.ShowView(latestView);
            }
            catch (Exception)
            {
                Application.Exit();
            }
        }
    }
}