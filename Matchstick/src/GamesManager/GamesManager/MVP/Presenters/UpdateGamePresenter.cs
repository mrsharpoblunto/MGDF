using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class UpdateGamePresenter: PresenterBase<IProgressView>
    {
        private event EventHandler OnComplete;

        private readonly IGame _game;
        private readonly int _pendingOperationCount;
        private readonly Thread _workerThread;

        private UpdateGamePresenter(IGame game, IProgressView view, EventHandler handler): base(view)
        {
            OnComplete += handler;
            _game = game;

            View.Invoke(() =>
                            {
                                View.Closed += View_Closed;
                                View.OnPause += View_OnPause;
                                View.OnResume += View_OnResume;
                                View.AllowPauseOrResume = true;
                                View.Title = "Updating " + _game.Name;
                                View.Details = "Updating " + _game.Name + ", please wait...";
                                View.ShowProgress(0, 100);
                            });
        }

        private void View_Closed(object sender, EventArgs e)
        {
            if (_workerThread != null && _workerThread.ThreadState == ThreadState.Running) _workerThread.Abort();
            View.OnPause -= View_OnPause;
            View.OnResume -= View_OnResume;
            View.Closed -= View_Closed;
        }

        public UpdateGamePresenter(IGame game, List<PendingOperation> pendingOperations, IProgressView view, EventHandler handler) : this(game,view,handler)
        {
            _pendingOperationCount = pendingOperations.Count;
            _workerThread = new Thread(ResumeUpdate);
            _workerThread.Start(pendingOperations);
        }

        public UpdateGamePresenter(IGame game, List<GameVersionUpdate> updates, IProgressView view, EventHandler handler)
            : this(game, view, handler)
        {
            _pendingOperationCount = updates.Count;
            _workerThread = new Thread(StartUpdate);
            _workerThread.Start(updates);
        }

        private void View_OnPause(object sender, EventArgs e)
        {
            GamesManagerClient.Instance.PausePendingOperations(_game);
        }

        private void View_OnResume(object sender, EventArgs e)
        {
            GamesManagerClient.Instance.ResumePendingOperations(_game);
        }

        private void StartUpdate(object args)
        {
            var updates = (List<GameVersionUpdate>) args;
            if (GamesManagerClient.Instance.Update(_game, updates))
            {
                DoWork();
            }
            else
            {
                if (OnComplete != null)
                {
                    OnComplete(this, new EventArgs());
                }
            }
        }

        private void ResumeUpdate(object args)
        {
            var pendingOperations = (List<PendingOperation>)args;

            //if it was pausing, wait until it has actually paused before resuming.
            while (pendingOperations.Count>0 && pendingOperations[0].Status == PendingOperationStatus.Pausing)
            {
                pendingOperations = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid).PendingOperations;
                Thread.Sleep(500);
            }

            if (pendingOperations.Count > 0 && pendingOperations[0].Status == PendingOperationStatus.Paused)
            {
                GamesManagerClient.Instance.ResumePendingOperations(_game);
            }
            DoWork();
        }

        private void DoWork()
        {
            View.Invoke(() => View.Paused = false);
            try 
            {
                var info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);

                while (info!=null && info.PendingOperations.Count>0)
                {
                    InstalledGameInfo gameInfo = info;
                    View.Invoke(() =>
                                    {
                                        View.Paused = gameInfo.PendingOperations[0].Status==PendingOperationStatus.Paused;
                                        View.Details = gameInfo.PendingOperations[0].Name + " " + (_pendingOperationCount - gameInfo.PendingOperations.Count + 1) + " of " + _pendingOperationCount + "...";
                                        View.ShowProgress(gameInfo.PendingOperations[0].Progress,gameInfo.PendingOperations[0].Total);
                                    });
                    Thread.Sleep(500);
                    info = GamesManagerClient.Instance.GetInstalledGameInfo(_game.Uid);
                }


                if (info==null || info.InstallState!=InstallState.Installed)
                {
                    Message.Show("This game failed to update, please check the logs for details.", "Update failed");
                }
                else
                {
                    //if the version after the update is the same as the version before, then no updates got applied properly.
                    if (_game.Version == new Version(info.Version))
                    {
                        Message.Show("This game failed to update, please check the logs for details.", "Update failed");                        
                    }
                }


                View.Invoke(() => View.HideProgress());

                if (OnComplete!=null)
                {
                    OnComplete(this,new EventArgs());
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
    }
}