using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Timers;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Helpers;
using MGDF.GamesManager.Model.Services;
using Game = MGDF.GamesManager.GameSource.Contracts.Entities.Game;

namespace MGDF.GamesManager.Model.ServiceModel
{
    public class GamesManagerService : IDisposable
    {
        private const int MaximumTimeBetweenHeartbeats = 30;

        private static GamesManagerService _instance;
        private static readonly object _instanceLock = new object();
        private readonly object _heartbeatLock = new object();

        private readonly InstalledGamesList _installedGamesList;
        private readonly StatisticsManager _statisticsManager;

        private Thread _heartBeatThread;
        private DateTime _lastHeartBeat;

        private bool _pendingShutDown;

        public static GamesManagerService Instance
        {
            get
            {
                if (_instance == null)
                {
                    lock (_instanceLock)
                    {
                        if (_instance == null)
                        {
                            _instance = new GamesManagerService();
                        }
                    }
                }
                return _instance;
            }
        }

        public bool HasHeartBeat
        {
            get
            {
                return (new TimeSpan(TimeService.Current.Now.Ticks - _lastHeartBeat.Ticks).TotalSeconds <= MaximumTimeBetweenHeartbeats);
            }
        }

        private GamesManagerService()
        {
            _installedGamesList = new InstalledGamesList();
            _statisticsManager = new StatisticsManager();

            HeartBeat();
        }

        public void HeartBeat()
        {
            if (_heartBeatThread == null)
            {
                lock (_heartbeatLock)
                {
                    if (_heartBeatThread == null)
                    {
                        _heartBeatThread = new Thread(CheckHeartBeat);
                        _heartBeatThread.Start();
                    }
                }
            }
            _lastHeartBeat = TimeService.Current.Now;
        }

        public void Dispose()
        {
            _pendingShutDown = true;
            try
            {
                _installedGamesList.PauseAll();
                _statisticsManager.Dispose();
                if (_heartBeatThread != null)
                {
                    lock (_heartbeatLock)
                    {
                        if (_heartBeatThread != null)
                        {
                            _heartBeatThread.Abort();
                            _heartBeatThread.Join();
                        }
                    }
                }

                _installedGamesList.WaitUntilAllPaused();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Error Disposing GamesManagerService");    
            }

            lock (_instanceLock)
            {
                _instance = null;
            }
        }

        public void Update(string gameUid, List<GameVersionUpdate> versions, List<string> errors)
        {
            if (_pendingShutDown)
            {
                errors.Add("Shutdown pending");
                return;
            }
            HeartBeat();

            var item = _installedGamesList.Get(gameUid);
            item.Update(versions,errors);
        }

        public void Install(string installSource, List<string> errors)
        {
            if (_pendingShutDown)
            {
                errors.Add("Shutdown pending");
                return;
            }
            HeartBeat();

            var installer = EntityFactory.Current.CreateGameInstall(installSource);
            if (installer.IsValid)
            {
                var item = _installedGamesList.Get(installer.Game.Uid);
                item.Install(installer, errors);
            }
            else
            {
                errors.Add("Invalid game installer file");
            }
        }

        public void Uninstall(string gameUid, List<string> errors)
        {
            if (_pendingShutDown)
            {
                errors.Add("Shutdown pending");
                return;
            }
            HeartBeat();

            var item = _installedGamesList.Get(gameUid);
            item.Uninstall(gameUid, errors);
        }

        public void ResumePendingOperations(string gameUid)
        {
            if (_pendingShutDown) return;
            HeartBeat();

            var item = _installedGamesList.Get(gameUid);
            item.Start();
        }

        public void PausePendingOperations(string gameUid)
        {
            if (_pendingShutDown) return;
            HeartBeat();

            var item = _installedGamesList.Get(gameUid);
            item.Pause();
        }

        public InstalledGameInfo GetGameInfo(string gameUid)
        {
            if (!_pendingShutDown)HeartBeat();
            var item = _installedGamesList.Get(gameUid);
            return item.GetInfo();
        }

        public void SubmitStatistics(string gameUid,string statisticsFile)
        {
            if (_pendingShutDown) return;
            HeartBeat();

            if (FileSystem.Current.FileExists(statisticsFile))
            {
                //load and parse stats
                var game = _installedGamesList.Get(gameUid).Game;
                if (game != null && !string.IsNullOrEmpty(game.StatisticsService) && !string.IsNullOrEmpty(game.StatisticsPrivacyPolicy))
                {
                    try
                    {
                        var session = new StatisticsSession(gameUid, game.StatisticsService, statisticsFile);
                        _statisticsManager.SendStatisticsSession(session);
                        FileSystem.Current.GetFile(statisticsFile).DeleteWithTimeout();
                    }
                    catch (Exception ex)
                    {
                        Logger.Current.Write(ex,"Invalid statistics file");
                    }
                }
            }
        }

        private void CheckHeartBeat()
        {
            try
            {
                while (true)
                {
                    if (!HasHeartBeat)
                    {
                        Logger.Current.Write(LogInfoLevel.Info, "GamesManager Service has lost its heartbeat, preparing to pause all pending operations...");
                        _installedGamesList.PauseAll();
                        _heartBeatThread = null;
                        break;
                    }
                    Thread.Sleep(1000);
                }
            }
            catch (ThreadAbortException)
            {
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unexpected error checking GamesManager service heartbeat");                
            }
        }
    }
}