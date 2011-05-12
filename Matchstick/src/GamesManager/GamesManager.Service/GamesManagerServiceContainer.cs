using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceProcess;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Service
{
    enum ServiceState
    {
        Stopped,
        Starting,
        Started,
        Stopping
    }

    public class GamesManagerServiceContainer
    {
        private ServiceState _state;
        private ServiceHost _host;
        private readonly object _lock = new object();

        public GamesManagerServiceContainer(bool initializeServices)
        {
            if (initializeServices)
            {
                TimeService.Current = new TimeService();
                FileSystem.Current = new FileSystem();
                Registry.Current = new Registry();
                GameExplorer.Current = new GameExplorer();
                IconManager.Current = new IconManager();
                ShortcutManager.Current = new ShortcutManager();
                ServiceManager.Current = new ServiceManager();
                HttpRequestManager.Current = new HttpRequestManager();
                EnvironmentSettings.Current = new EnvironmentSettings();
                EntityFactory.Current = new EntityFactory();
                ArchiveFactory.Current = new ArchiveFactory();
                IdentityGenerator.Current = new IdentityGenerator();
                ProcessManager.Current = new ProcessManager();
                Logger.Current = new Logger(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "GamesManagerServiceLog.txt"));
            }

            AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;

            _state = ServiceState.Stopped;
        }

        public void OnStart(string[] args)
        {
            Logger.Current.Write(LogInfoLevel.Info, "Starting up GamesManager Admin Service...");
            StartUp();
        }

        private void StartUp()
        {
            try
            {
                lock (_lock)
                {
                    if (_state==ServiceState.Stopped)
                    {
                        _state = ServiceState.Starting;
                    }
                    else
                    {
                        return;
                    }
                }

                if (_state==ServiceState.Starting)
                {
                    Constants.CreateRequiredCommonDirectories();

                    if (Config.AutoUpdateFramework) FrameworkUpdateManager.Instance.Start();

                    _host = new ServiceHost(typeof(GamesManagerAdminService), new Uri(GamesManagerAdminService.BaseAddress));
                    _host.AddServiceEndpoint(typeof(IGamesManagerAdminService), new NetNamedPipeBinding(), GamesManagerAdminService.EndPointAddress);
                    _host.Open();

                    _state =ServiceState.Started;
                    Logger.Current.Write(LogInfoLevel.Info, "GamesManager Admin Service started up.");
                }
                else
                {
                    Logger.Current.Write(LogInfoLevel.Info, "GamesManager Admin Service already running.");
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Failed to start GamesManager Admin Service");
            }
        }

        public bool OnPowerEvent(PowerBroadcastStatus powerStatus)
        {
            switch (powerStatus)
            {
                case PowerBroadcastStatus.QuerySuspend:
                    // The system has requested permission to suspend the
                    // computer. We return true to grant our permission.
                    return true;
                case PowerBroadcastStatus.Suspend:
                    // The computer is about to enter a suspended state.
                    Logger.Current.Write(LogInfoLevel.Info, "Computer entering suspended state, shutting down GamesManager Admin Service...");
                    ShutDown();
                    break;
                case PowerBroadcastStatus.ResumeAutomatic:
                case PowerBroadcastStatus.ResumeCritical:
                case PowerBroadcastStatus.ResumeSuspend:
                    // The system has resumed operation after being suspended.
                    Logger.Current.Write(LogInfoLevel.Info,"Computer resuming from suspended state, starting up GamesManager Admin Service...");
                    StartUp();
                    break;
                default:
                    break;
            }

            return false;
        }

        private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Logger.Current.Write(LogInfoLevel.Error, "Unexpected error in GamesManager Admin Service - " + e.ExceptionObject);
            Logger.Current.Write(LogInfoLevel.Info, "Attempting forced shutdown of GamesManager Admin Service...");
            ShutDown();
            Environment.Exit(0);
        }

        public void OnShutdown()
        {
            Logger.Current.Write(LogInfoLevel.Info, "Shutting down GamesManager Admin Service...");
            ShutDown();
        }

        public void OnStop()
        {
            Logger.Current.Write(LogInfoLevel.Info, "Stopping GamesManager Admin Service...");
            ShutDown();
        }

        private void ShutDown()
        {
            lock (_lock)
            {
                if (_state==ServiceState.Started)
                {
                    _state = ServiceState.Stopping;
                }
                else
                {
                    return;
                }
            }

            if (_state==ServiceState.Stopping)
            {
                try
                {
                    GamesManagerService.Instance.Dispose();
                    FrameworkUpdateManager.Instance.Dispose();
                    _host.Close();
                    _state = ServiceState.Stopped;
                    Logger.Current.Write(LogInfoLevel.Info, "GamesManager Admin Service shut down.");
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex, "Error shutting down GamesManager Admin Service");
                }
            }
        }
    }
}