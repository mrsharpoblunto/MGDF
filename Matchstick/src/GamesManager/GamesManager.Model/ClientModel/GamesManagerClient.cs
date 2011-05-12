using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceProcess;
using System.Threading;
using ACorns.WCF.DynamicClientProxy;
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

        private readonly IGamesManagerAdminService _adminService;
        private readonly Thread _servicePollThread;

#if RELEASE
        private static IService _adminServiceController;
#endif

        public static Func<IGamesManagerAdminService> ServiceFactory = () =>
                                                                           {
                                                                               EnsureServiceStarted();
                                                                               return WCFClientProxy<IGamesManagerAdminService>.GetReusableFaultUnwrappingInstance(new NetNamedPipeBinding(), new EndpointAddress(GamesManagerAdminService.BaseAddress+"/"+ GamesManagerAdminService.EndPointAddress));
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
                InvokeService(() => _adminService.Heartbeat(new HearbeatRequest()));
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
            (_adminService as IDisposable).Dispose();
        }

        public InstalledGameInfo GetInstalledGameInfo(string uid)
        {
            GetInstalledGameInfoResponse response=null;
            if (InvokeService(() => response = _adminService.GetInstalledGameInfo(new GetInstalledGameInfoRequest { GameUid = uid })) && response != null)
            {
                return response.Info;
            }

            return null;
        }

        public bool Update(IGame game, List<GameVersionUpdate> versions)
        {
            return InvokeService(() => _adminService.UpdateGame(new UpdateGameRequest { GameUid = game.Uid, GameVersions = versions }));           
        }

        public bool Install(string installSource)
        {
            return InvokeService(() =>_adminService.InstallLocalGame(new InstallLocalGameRequest { InstallSource = installSource }));
        }

        public bool Uninstall(IGame game)
        {
            return InvokeService(() => _adminService.UninstallGame(new UninstallGameRequest { GameUid = game.Uid}));
        }

        public bool ResumePendingOperations(IGame game)
        {
            return InvokeService(() => _adminService.ResumePendingOperations(new ResumePendingOperationsRequest() { GameUid = game.Uid }));
        }

        public bool PausePendingOperations(IGame game)
        {
            return InvokeService(() => _adminService.PausePendingOperations(new PausePendingOperationsRequest { GameUid = game.Uid }));
        }

        public bool SubmitStatistics(IGame game)
        {
            return InvokeService(() => _adminService.SubmitStatistics(new SubmitStatisticsRequest { GameUid = game.Uid, StatisticsFile = Constants.UserStatistics(game.Uid) }));
        }

        #region helpers

        private bool InvokeService<RESPONSE>(Func<RESPONSE> serviceCallHandler) where RESPONSE : ResponseBase
        {
            try
            {
                //always ensure the admin service is running before calling out to it.
                EnsureServiceStarted();
                RESPONSE response = serviceCallHandler();
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