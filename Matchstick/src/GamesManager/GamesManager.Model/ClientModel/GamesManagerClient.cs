using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceProcess;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Messages;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Model.ClientModel
{
    public class GamesManagerClient: IDisposable
    {
        private static GamesManagerClient _instance;
        private static readonly object _instanceLock = new object();

        public static bool HasInstance()
        {
            return _instance != null;
        }

        public static GamesManagerClient Instance
        {
            get
            {
                if (_instance == null)
                {
                    lock (_instanceLock)
                    {
                        if (_instance == null)
                        {
                            _instance = new GamesManagerClient();
                        }
                    }
                }
                return _instance;
            }
        }

        private readonly IWCFClient<IGamesManagerAdminService> _adminService;
        private readonly Thread _servicePollThread;

#if RELEASE
        private static IService _adminServiceController;
#endif

        public static Func<IWCFClient<IGamesManagerAdminService>> ServiceFactory = () =>
                                                                           {
                                                                               EnsureServiceStarted();
                                                                               var factory = new ChannelFactory<IGamesManagerAdminService>(new NetNamedPipeBinding(), new EndpointAddress(GamesManagerAdminService.BaseAddress + "/" + GamesManagerAdminService.EndPointAddress));
                                                                               return new WCFClient<IGamesManagerAdminService>(factory);
                                                                           };

        public GamesManagerClient()
        {
            _adminService = ServiceFactory();

            //ensure the admin service stays up and running for the duration of the application, otherwise if its not contacted for 30 seconds it will auto shutdown.
            _servicePollThread = new Thread(DoAdminServicePolling);
            _servicePollThread.Start();
        }

        private static void EnsureServiceStarted()
        {
#if RELEASE
            if (_adminServiceController==null) 
            {
                _adminServiceController = ServiceManager.Current.GetService("MGDF.GamesManager.Service");
            }

            _adminServiceController.Start();
            _adminServiceController.WaitForStatus(ServiceControllerStatus.Running);
#endif
        }

        private void DoAdminServicePolling()
        {
            while (true)
            {
                Thread.Sleep(10000);

                //do a heartbeat ~10 seconds to keep the service alive.
                InvokeService(s => s.Heartbeat(new HearbeatRequest()));
            }
        }

        public void Dispose()
        {
            //clean up the admin service proxy and heartbeat
            if (_servicePollThread != null)
            {
                _servicePollThread.Abort();
                _servicePollThread.Join();
            }
        }

        public InstalledGameInfo GetInstalledGameInfo(string uid)
        {
            GetInstalledGameInfoResponse response=null;
            if (InvokeService(s => response = s.GetInstalledGameInfo(new GetInstalledGameInfoRequest { GameUid = uid })) && response != null)
            {
                return response.Info;
            }

            return null;
        }

        public bool Update(IGame game, List<GameVersionUpdate> versions)
        {
            return InvokeService(s=>s.UpdateGame(new UpdateGameRequest { GameUid = game.Uid, GameVersions = versions }));           
        }

        public bool Install(string installSource)
        {
            return InvokeService(s=>s.InstallLocalGame(new InstallLocalGameRequest { InstallSource = installSource }));
        }

        public bool Uninstall(IGame game)
        {
            return InvokeService(s=>s.UninstallGame(new UninstallGameRequest { GameUid = game.Uid}));
        }

        public bool ResumePendingOperations(IGame game)
        {
            return InvokeService(s => s.ResumePendingOperations(new ResumePendingOperationsRequest() { GameUid = game.Uid }));
        }

        public bool PausePendingOperations(IGame game)
        {
            return InvokeService(s => s.PausePendingOperations(new PausePendingOperationsRequest { GameUid = game.Uid }));
        }

        public bool SubmitStatistics(IGame game)
        {
            return InvokeService(s => s.SubmitStatistics(new SubmitStatisticsRequest { GameUid = game.Uid, StatisticsFile = Constants.UserStatistics(game.Uid) }));
        }

        #region helpers

        private bool InvokeService<RESPONSE>(Func<IGamesManagerAdminService,RESPONSE> serviceCallHandler) where RESPONSE : ResponseBase
        {
            try
            {
                //always ensure the admin service is running before calling out to it.
                EnsureServiceStarted();
                RESPONSE response = _adminService.Use(serviceCallHandler);
                if (response.Errors.Count>0)
                {
                    foreach (var error in response.Errors)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, error);
                    }

                    return false;
                }
                return true;
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unexpected GamesManager Service Error");
                return false;
            }
        }

        #endregion

    }
}