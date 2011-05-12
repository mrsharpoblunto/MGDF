using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class InstallLocalGamePresenter: PresenterBase<IProgressView>
    {  
        private Thread _workerThread;
        private readonly IGame _game;
        private readonly string _installer;

        public InstallLocalGamePresenter(IGameInstall installer)
        {
            View.Shown+=View_Shown;
            View.Closing += View_Closing;
            View.AllowPauseOrResume = false;

            if (installer.ErrorCollection.Count == 0)
            {
                _installer = installer.InstallerFile;
                _game = installer.Game;
                if (installer.Game.GameIconData != null)
                {
                    View.GameIcon = Image.FromStream(new MemoryStream(installer.Game.GameIconData));
                }
            }

            View.HideProgress();
        }

        void View_Closing(object sender, CancelEventArgs e)
        {
            if (_workerThread != null && _workerThread.ThreadState == ThreadState.Running) _workerThread.Abort();
        }

        void View_Shown(object sender, EventArgs e)
        {
            if (_game == null)
            {
                Message.Show("This installer package is invalid", "Invalid installer");
                CloseView();
            }
            else
            {
                View.Title = "Installing " + _game.Name;
                View.Details = "Installing " + _game.Name + ", please wait...";
                _workerThread = new Thread(DoWork);
                _workerThread.Start();
            }
        }

        private void DoWork()
        {
            try
            {
                var info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);

                if (info==null)
                {
                    View.Invoke(() =>
                                    {
                                        var controller = new SubmitErrorPresenter("Unexpected error in GamesManager", "Unable to contact the GamesManager Admin service");
                                        controller.ShowView(View);
                                    });
                }
                else if (info.InstallState == InstallState.NotInstalled)
                {
                    GamesManagerClient.Instance.Install(_installer);

                    info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);
                    while (info!=null && info.PendingOperations.Count > 0)
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
                    else if (info.InstallState != InstallState.Installed)
                    {
                        View.Invoke(() => Message.Show("This game failed to install, please check the logs for details.", "Install failed"));
                    }
                    View.Invoke(CloseView);
                }
                else if (info.InstallState == InstallState.Updating)
                {
                    //finish off any updates or paused installs
                    new UpdateGamePresenter(_game, info.PendingOperations, View, UpdateGamePresenter_OnComplete);
                }
                else
                {
                    View.Invoke(() => Message.Show("This game cannot be installed as it has already been installed", "Game already installed"));
                    View.Invoke(CloseView);
                }
            }
            catch (ThreadAbortException)
            {
                
            }
            catch (Exception ex)
            {
                View.Invoke(() => Program.ShowUnhandledError(ex));
            }
        }

        private void UpdateGamePresenter_OnComplete(object sender,EventArgs e)
        {
            ShortcutManager.RefreshDesktop();
            View.Invoke(CloseView);
        }
    }
}