using System;
using System.Collections.Generic;
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
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    public class UninstallGamePresenter: PresenterBase<IProgressView>
    {
        private readonly IGame _game;
        private Thread _workerThread;

        public UninstallGamePresenter(string uid)
        {
            View.Shown+=View_Shown;
            View.Closing += View_Closing;
            View.AllowPauseOrResume = false;

            try 
            {
                _game = EntityFactory.Current.CreateGame(FileSystem.Combine(Constants.GameDir(uid),Constants.GameConfig));
                if (_game.GameIconData != null)
                {
                    View.GameIcon = Image.FromStream(new MemoryStream(_game.GameIconData));
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
                View.Title = "Uninstalling " + _game.Name;
                View.Details = "Uninstalling " + _game.Name + ", please wait...";
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
                                    });
                }
                else if (info.InstallState == InstallState.Updating && info.PendingOperations.Count > 0 && info.PendingOperations[0].Name == "Uninstalling")
                {
                    View.Invoke(() => Message.Show("This game is already being uninstalled", "Already uninstalling"));
                }
                else if (info.InstallState != InstallState.NotInstalled)
                {
                    bool preserveUserData = Message.Show("Do you also want to remove all of the saves and preferences for this game?", "Remove saved games", MessageBoxButtons.YesNo) == DialogResult.No;

                    if (!preserveUserData && FileSystem.Current.DirectoryExists(Constants.GameUserDir(_game.Uid)))
                    {
                        FileSystem.Current.GetDirectory(Constants.GameUserDir(_game.Uid)).DeleteWithTimeout(); //remove all settings for this game
                    }
                    GamesManagerClient.Instance.Uninstall(_game);

                    info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);
                    while (info !=null && info.PendingOperations.Count > 0)
                    {
                        InstalledGameInfo gameInfo = info;
                        View.Invoke(() =>
                                        {
                                            View.Details = gameInfo.PendingOperations[0].Name + "...";
                                            View.ShowProgress(gameInfo.PendingOperations[0].Progress, gameInfo.PendingOperations[0].Total);
                                        });
                        Thread.Sleep(500);
                        info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);
                    }
                    ShortcutManager.RefreshDesktop();
                    if (info==null)
                    {
                        View.Invoke(() =>
                                        {
                                            var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", "Unable to contact the GamesManager Admin service");
                                            controller.ShowView(View);
                                        });
                    }
                    else if(info.InstallState != InstallState.NotInstalled)
                    {
                        View.Invoke(() => Message.Show("This game failed to uninstall, please check the logs for details.", "Uninstall failed"));
                    }
                }
                View.Invoke(CloseView);
            }
            catch (ThreadAbortException)
            {
                
            }
            catch (Exception ex)
            {
                View.Invoke(() => Program.ShowUnhandledError(ex));
            }
        }
    }
}