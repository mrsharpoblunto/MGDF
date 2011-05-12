using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
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
            Logger.Current = new Logger(Path.Combine(EnvironmentSettings.Current.UserDirectory, "GamesManagerLog.txt"));
            EntityFactory.Current = new EntityFactory();
            ArchiveFactory.Current = new ArchiveFactory();
            HttpRequestManager.Current = new HttpRequestManager();
            ProcessManager.Current = new ProcessManager();
            Constants.CreateRequiredUserDirectories();

            ViewImpl.RegisterViews();

            try
            {
                using (new CleanupScope(Cleanup))
                {
                    return RunMain();
                }
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
            Application.ThreadException += Application_ThreadException;
            var commandLine = new CommandLineParser(Environment.GetCommandLineArgs());

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (commandLine[Constants.GamesManagerArguments.BootArgument]!=null)
            {
                string uid = commandLine[Constants.GamesManagerArguments.BootArgument];
                var presenter = new LaunchGamePresenter(uid, commandLine[Constants.GamesManagerArguments.NoUpdateCheckArgument] == null, false);
                Application.Run(presenter.View as Form);
            }
            else if (commandLine[Constants.GamesManagerArguments.InstallArgument] != null)
            {
                var splashPresenter = new InstallSplashScreenPresenter(commandLine[Constants.GamesManagerArguments.InstallArgument]);
                if (splashPresenter.Installer != null)
                {
                    Application.Run(splashPresenter.View as Form);
                    if (splashPresenter.DoInstall)
                    {
                        var installPresenter = new InstallLocalGamePresenter(splashPresenter.Installer);
                        Application.Run(installPresenter.View as Form);
                    }
                }
            }
            else if (commandLine[Constants.GamesManagerArguments.UninstallArgument] != null)
            {
                var presenter = new UninstallGamePresenter(commandLine[Constants.GamesManagerArguments.UninstallArgument]);
                Application.Run(presenter.View as Form);
            }
            else if (commandLine[Constants.GamesManagerArguments.CheckUpdatesArgument] != null)
            {
                var presenter = new LaunchGamePresenter(commandLine[Constants.GamesManagerArguments.CheckUpdatesArgument],true,true);
                presenter.ShowView();
                Application.Run(presenter.View as Form);
            }
            else if (commandLine[Constants.GamesManagerArguments.CheckUninstallArgument] != null)
            {
                IDirectory gamesDirectory = FileSystem.Current.GetDirectory(Constants.GamesBaseDir);
                var games = gamesDirectory.GetSubDirectories("*")
                    .Where(d =>!d.Name.Equals("core",StringComparison.InvariantCultureIgnoreCase) && !d.Name.Equals("Downloads",StringComparison.InvariantCultureIgnoreCase))
                    .Map(d => d.Name);

                if (games.Count > 0)
                {
                    var presenter = new CheckUninstallPresenter(games);
                    presenter.ShowView();
                    Application.Run(presenter.View as Form);
                    return presenter.Uninstall ? 0 : -1;
                }
            }
            else
            {
                ViewFactory.Current.CreateView<IMessage>().Show("Unknown arguments", "Unknown arguments");
                return -1;
            }

            return 0;
        }

        public static void Application_ThreadException(object sender, ThreadExceptionEventArgs e)
        {
            ShowUnhandledError(e.Exception);
        }

        public static void ShowUnhandledError(Exception e)
        {
            Cleanup();

            try
            {
                IView latestView = ViewFactory.Current.LatestView;
                Logger.Current.Write(e, "Unexpected error in GamesManager");
                var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", e.ToString());
                controller.View.Closed += (s, args) => Application.Exit();
                controller.ShowView(latestView);
            }
            catch (Exception)
            {
                Application.Exit();
            }
        }

        private static void Cleanup()
        {
            try
            {
                if (GamesManagerClient.HasInstance())
                {
                    GamesManagerClient.Instance.Dispose();
                }
            }
            catch (Exception)
            {
            }
        }
    }

    class CleanupScope: IDisposable
    {
        private readonly Action _cleanupHandler;

        public CleanupScope(Action cleanupHandler)
        {
            _cleanupHandler = cleanupHandler;
        }

        public void Dispose()
        {
            _cleanupHandler();
        }
    }
}