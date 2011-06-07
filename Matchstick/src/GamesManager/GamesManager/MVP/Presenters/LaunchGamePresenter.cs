using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    public class LaunchGamePresenter: PresenterBase<IProgressView>
    {
        private readonly IGame _game;
        private bool _sendStatistics;
        private readonly bool _checkForUpdates;
        private readonly bool _updateCheckOnly;
        private Thread _workerThread;

        public LaunchGamePresenter(string uid,bool checkForUpdates,bool updateCheckOnly)
        {
            View.Shown+=View_Shown;
            View.Closing += View_Closing;
            _checkForUpdates = checkForUpdates;
            _updateCheckOnly = updateCheckOnly;

            try 
            {
                _game = EntityFactory.Current.CreateGame(FileSystem.Combine(Constants.GameDir(uid),Constants.GameConfig));
                if (_game.GameIconData != null)
                {
                    View.GameIcon = Image.FromStream(new MemoryStream(_game.GameIconData));
                }

                //only check for updates once a day
                IFile lastUpdate = FileSystem.Current.GetFile(Constants.UserGameLastUpdateFile(_game.Uid));
                if (lastUpdate.Exists && lastUpdate.LastWriteTimeUtc.DayOfYear == TimeService.Current.Now.DayOfYear && lastUpdate.LastWriteTimeUtc.Year == TimeService.Current.Now.Year)
                {
                    _checkForUpdates = false;
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unexpected error loading game information for '"+uid+"'");
                _game = null;
            }

            View.HideProgress();
        }

        void View_Closing(object sender, EventArgs e)
        {
            if (_workerThread != null && _workerThread.ThreadState == ThreadState.Running) _workerThread.Abort();
        }

        void View_Shown(object sender, EventArgs e)
        {
            IFile gameLock = FileSystem.Current.GetFile(Constants.UserGameLockFile(_game.Uid));
            if (gameLock.Exists)
            {
                Message.Show("This game is already running", "Game already running");
                CloseView();
            }
            else if (_game==null)
            {
                Message.Show("This game is not installed", "Game not installed");
                CloseView();
            }
            else if (_game.ErrorCollection.Count > 0)
            {
                Message.Show("This game is invalid", "Game invalid");
                CloseView();
            }
            else
            {
                View.Title = "Launching " + _game.Name;
                View.Details = "Launching " + _game.Name+ ", please wait...";
                _workerThread = new Thread(DoWork);
                _workerThread.Start();
            }
        }

        private void DoWork()
        {
            try
            {
                var info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);

                if (info == null)
                {
                    View.Invoke(() =>
                                    {
                                        var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", "Unable to contact the GamesManager Admin service");
                                        controller.ShowView(View);
                                        CloseView();
                                    });
                }
                else if (info.InstallState == InstallState.Installed && _checkForUpdates && !string.IsNullOrEmpty(_game.GameSourceService))
                {
                    View.Invoke(() =>
                                    {
                                        View.Title = "Checking for updates...";
                                        View.Details = "Checking for updates...";
                                    });
                    var gameSourceClient = new GameSourceClient();
                    var errors = new List<string>();
                    var updates = gameSourceClient.GetGameUpdates(_game, GetCredentials, GetStatisticsPermission, errors);

                    if (errors.Count != 0)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, errors[0]);
                        DoLaunch(info);
                    }
                    else
                    {
                        if (updates.Count > 0 && ViewFactory.Current.ConfirmYesNo("Update available", "An update is available, would you like to download it now?"))
                        {
                            new UpdateGamePresenter(_game, updates, View, UpdateGamePresenter_OnComplete);
                        }
                        else
                        {
                            DoLaunch(info);
                        }
                    }
                }
                //if we're already updating, just resume where we left off, or show the current progress.
                else if (info.InstallState == InstallState.Updating)
                {
                    if (info.PendingOperations[0].Name == "Uninstalling")
                    {
                        View.Invoke(() =>
                                        {
                                            Message.Show("This game cannot be run as it is being uninstalled", "Game is being uninstalled");
                                            CloseView();
                                        });
                    }
                    else
                    {
                        new UpdateGamePresenter(_game, info.PendingOperations, View, UpdateGamePresenter_OnComplete);
                    }
                }
                //otherwise just launch the game
                else
                {
                    DoLaunch(info);
                }
            }
            catch (ThreadAbortException)
            {
                
            }
            catch (Exception ex)
            {
                View.Invoke(() =>Program.ShowUnhandledError(ex));
            }
        }

        private void UpdateGamePresenter_OnComplete(object sender, EventArgs e)
        {
            ShortcutManager.RefreshDesktop();

            //update the last updated file.
            IFile lastUpdate = FileSystem.Current.GetFile(Constants.UserGameLastUpdateFile(_game.Uid));
            lastUpdate.WriteText(TimeService.Current.Now.ToString());

            var info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);
            if (info == null)
            {
                View.Invoke(() =>
                                {
                                    var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", "Unable to contact the GamesManager Admin service");
                                    controller.View.Closed += (s, ev) => CloseView();
                                    controller.ShowView(View);
                                });
            }

            DoLaunch(info);
        }

        private void DoLaunch(InstalledGameInfo info)
        {
            if (_updateCheckOnly)
            {
                View.Invoke(CloseView);
                return;
            }

            View.Invoke(()=>
                            {
                                View.Title = "Launching " + _game.Name;
                                View.Details = "Launching " + _game.Name + ", please wait...";
                            });
            switch (info.InstallState)
            {
                case InstallState.NotInstalled:
                    View.Invoke(() =>
                                    {
                                        Message.Show("This game cannot be run as it is not installed", "Game not installed");
                                        CloseView();
                                    });
                    break;
                case InstallState.Error:
                    View.Invoke(() =>
                                    {
                                        Message.Show("This game cannot be run as it is not installed correctly, uninstalling and reinstalling the game may fix the problem.", "Game not installed correctly");
                                        CloseView();
                                    });
                    break;
                case InstallState.Updating:
                    View.Invoke(() =>
                                    {
                                        Message.Show("This game cannot be run as it is currently being updated, please wait for the update to finish.", "Game being updated");
                                        CloseView();
                                    });
                    break;
                default:
                    var settings = SettingsManager.Instance.Games.Find(g => g.GameUid == _game.Uid);
                    _sendStatistics = GameSourceClient.GetStatisticsPermission(_game, settings, GetStatisticsPermission).StatisticsServiceEnabled.Value;
                    Launch(_game);
                    break;
            }
        }

        private bool GetStatisticsPermission(GetStatsPermissionEventArgs arg)
        {
            var presenter = new SendStatisticsPresenter(_game);
            View.Invoke(()=>presenter.ShowView(View));
            return presenter.UserPermissionGranted;
        }

        public bool GetCredentials(GetCredentialsEventArgs args)
        {
            var presenter = new GetCredentialsPresenter(_game,args);
            View.Invoke(()=>presenter.ShowView(View));
            return presenter.OK;
        }

        public void Launch(IGame game)
        {
            //lock the game so that it cannot be launched more than once at a time.
            IFile gameLock = FileSystem.Current.GetFile(Constants.UserGameLockFile(_game.Uid));
            gameLock.WriteText(TimeService.Current.Now.ToString());

            ProcessManager.Current.StartProcess(Constants.MGDFExecutable, Constants.MGDFBootArguments(game.Uid), GameExited, game);
            View.Invoke(()=>View.Hide());
        }

        private void GameExited(object context, int exitCode)
        {
            IGame game = (IGame) context;

            try
            {
                IFile gameLock = FileSystem.Current.GetFile(Constants.UserGameLockFile(game.Uid));
                gameLock.DeleteWithTimeout();
            }
            catch(Exception ex)
            {
                Logger.Current.Write(ex,"Unable to delete lock file");
            }

            if (exitCode != 0)
            {
                View.Invoke(() =>
                                {
                                    SubmitCoreErrorPresenter presenter = new SubmitCoreErrorPresenter(game,game.Name + " has ended unexpectedly",
                                                                                                      "An unhandled exception or fatal MGDF error has occurred");
                                    presenter.ShowView(View);
                                    CloseView();
                                });
            }
            else
            {
                if (_sendStatistics)
                {
                    GamesManagerClient.Instance.SubmitStatistics(game);
                }
                View.Invoke(CloseView);
            }
        }
    }
}