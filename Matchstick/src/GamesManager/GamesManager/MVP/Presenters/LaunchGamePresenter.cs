using System;
using System.Collections.Generic;
using System.Configuration;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    public class LaunchGamePresenter: PresenterBase<IProgressView>
    {
        private readonly bool _checkForUpdates;
        private Thread _workerThread;

        public LaunchGamePresenter(bool checkForUpdates)
        {
            View.Shown+=View_Shown;
            View.Closing += View_Closing;
            _checkForUpdates = checkForUpdates;

            try 
            {
                if (Game.Current.GameIconData != null)
                {
                    View.GameIcon = Image.FromStream(new MemoryStream(Game.Current.GameIconData));
                }

                //only check for updates once a day
                IFile lastUpdate = FileSystem.Current.GetFile(Resources.UserGameLastUpdateFile(Game.Current.Uid));
                if (lastUpdate.Exists && lastUpdate.LastWriteTimeUtc.DayOfYear == TimeService.Current.Now.DayOfYear && lastUpdate.LastWriteTimeUtc.Year == TimeService.Current.Now.Year)
                {
                    _checkForUpdates = false;
                }
                else
                {
                    lastUpdate.WriteText("Last checked for updates on "+TimeService.Current.Now.ToShortDateString());
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unexpected error loading game information for '" + Game.Current.Uid + "'");
            }

            View.HideProgress();
        }

        void View_Closing(object sender, EventArgs e)
        {
            if (_workerThread != null && _workerThread.ThreadState == ThreadState.Running) _workerThread.Abort();
        }

        void View_Shown(object sender, EventArgs e)
        {
            View.Title = "Launching " + Game.Current.Name;
            View.Details = "Launching " + Game.Current.Name + ", please wait...";
            _workerThread = new Thread(DoWork);
            _workerThread.Start();
        }

        private void DoWork()
        {
            try
            {
                if (_checkForUpdates)
                {
                    View.Invoke(() =>
                    {
                        View.Title = "Checking for updates...";
                        View.Details = "Checking for updates...";
                    });

                    UpdateDownload frameworkUpdate=null;
                    if (Config.Current.AutoUpdateFramework)
                    {
                        frameworkUpdate = UpdateChecker.CheckForFrameworkUpdate();
                    }
                    UpdateDownload gameUpdate = UpdateChecker.CheckForGameUpdate(Game.Current);

                    if ((frameworkUpdate != null || gameUpdate != null) && GetUpdatePermission())
                    {
                        if (!UACControl.IsVistaOrHigher() && !UACControl.IsAdmin())
                        {
                            ViewFactory.Current.CreateView<IMessage>().Show("Updating requires administrator access", "Administrator accesss required");
                            _workerThread = null;
                            View.Invoke(CloseView);
                            return;
                        }

                        UACControl.RestartElevated(Resources.GamesManagerBootArguments(
                            gameUpdate!=null ? gameUpdate.Url : string.Empty, 
                            gameUpdate!=null ? gameUpdate.MD5 : string.Empty, 
                            frameworkUpdate!=null ? frameworkUpdate.Url : string.Empty,
                            frameworkUpdate != null ? frameworkUpdate.MD5 : string.Empty));
                        return;
                    }

                }

                Launch();
            }
            catch (ThreadAbortException)
            {
                
            }
            catch (Exception ex)
            {
                _workerThread = null;
                View.Invoke(() =>Program.ShowUnhandledError(ex));
            }
        }

        private bool GetUpdatePermission()
        {
            GetUpdatePermissionPresenter presenter = null;
            View.Invoke(() =>
            {
                presenter = new GetUpdatePermissionPresenter();
                presenter.ShowView(View);
            });
            return presenter.Update;
        }

        private bool GetStatisticsPermission(GetStatsPermissionEventArgs arg)
        {
            SendStatisticsPresenter presenter = null;
            View.Invoke(() =>
            {
                presenter = new SendStatisticsPresenter(Game.Current);
                presenter.ShowView(View);
            });
            return presenter.UserPermissionGranted;
        }

        public void Launch()
        {
            ProcessManager.Current.StartProcess(Resources.MGDFExecutable, Resources.CoreBootArguments(Game.Current.Uid), GameExited, Game.Current);
            View.Invoke(()=>View.Hide());
        }

        private void GameExited(object context, int exitCode)
        {
            Game game = (Game) context;

            if (exitCode != 0)
            {
                View.Invoke(() =>
                {
                    SubmitCoreErrorPresenter presenter = new SubmitCoreErrorPresenter(game,game.Name + " has ended unexpectedly",
                                                                                      "An unhandled exception or fatal MGDF error has occurred");
                    presenter.ShowView(View);
                });
            }
            else if (StatisticsSession.CanSendStatistics(Game.Current))
            {
                IFile statisticsFile = FileSystem.Current.GetFile(Resources.UserStatistics(Game.Current.Uid));
                if (statisticsFile.Exists)
                {
                    try
                    {
                        if (StatisticsSession.GetStatisticsPermission(Game.Current, GetStatisticsPermission))
                        {
                            StatisticsSession session = new StatisticsSession(Game.Current.Uid, Game.Current.StatisticsService, statisticsFile.FullName);
                            StatisticsServiceClient client = new StatisticsServiceClient(session);

                            List<String> errors = new List<string>();
                            client.SendStatistics(errors);
                            foreach (var error in errors)
                            {
                                Logger.Current.Write(LogInfoLevel.Error, error);
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.Current.Write(ex,"Unable to send statistics");
                    }
                    finally
                    {
                        statisticsFile.DeleteWithTimeout();
                    }
                }
            }

            View.Invoke(() =>
            {
                _workerThread = null;
                View.CloseView();
            });
        }
    }
}