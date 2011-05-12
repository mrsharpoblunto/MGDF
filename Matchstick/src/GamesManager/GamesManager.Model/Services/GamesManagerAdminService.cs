using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Messages;
using MGDF.GamesManager.Model.ServiceModel;

namespace MGDF.GamesManager.Model.Services
{
    public class GamesManagerAdminService: IGamesManagerAdminService
    {
        public const string BaseAddress = "net.pipe://localhost";
        public const string EndPointAddress = "GamesManagerAdminServicePipe";

        private delegate void MethodHandler<RESPONSE>(RESPONSE response);

        private static RESPONSE InvokeMethod<RESPONSE>(MethodHandler<RESPONSE> handler)
            where RESPONSE : ResponseBase, new()
        {
            RESPONSE response = new RESPONSE();
            try
            {
                handler(response);
            }
            catch (Exception ex)
            {
                response.Errors.Add("Unknown error");
                Logger.Current.Write(ex, "Error processing " + typeof(RESPONSE).Name);
            }

            return response;
        }

        public HearbeatResponse Heartbeat(HearbeatRequest request)
        {
            return InvokeMethod<HearbeatResponse>(response => GamesManagerService.Instance.HeartBeat());
        }

        public InstallGameResponse UpdateGame(UpdateGameRequest request)
        {
            return InvokeMethod<InstallGameResponse>(response => GamesManagerService.Instance.Update(request.GameUid, request.GameVersions, response.Errors));
        }

        public InstallGameResponse InstallLocalGame(InstallLocalGameRequest request)
        {
            return InvokeMethod<InstallGameResponse>(response => GamesManagerService.Instance.Install(request.InstallSource, response.Errors));
        }

        public UninstallGameResponse UninstallGame(UninstallGameRequest request)
        {
            return InvokeMethod<UninstallGameResponse>(response => GamesManagerService.Instance.Uninstall(request.GameUid, response.Errors));
        }

        public GetInstalledGameInfoResponse GetInstalledGameInfo(GetInstalledGameInfoRequest request)
        {
            return InvokeMethod<GetInstalledGameInfoResponse>(response => response.Info = GamesManagerService.Instance.GetGameInfo(request.GameUid));
        }

        public ResumePendingOperationsResponse ResumePendingOperations(ResumePendingOperationsRequest request)
        {
            return InvokeMethod<ResumePendingOperationsResponse>(response => GamesManagerService.Instance.ResumePendingOperations(request.GameUid));
        }

        public PausePendingOperationsResponse PausePendingOperations(PausePendingOperationsRequest request)
        {
            return InvokeMethod<PausePendingOperationsResponse>(response => GamesManagerService.Instance.PausePendingOperations(request.GameUid));
        }

        public SubmitStatisticsResponse SubmitStatistics(SubmitStatisticsRequest request)
        {
            return InvokeMethod<SubmitStatisticsResponse>(response => GamesManagerService.Instance.SubmitStatistics(request.GameUid,request.StatisticsFile));
        }
    }
}
