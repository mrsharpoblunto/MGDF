using System;
using System.Linq;
using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.Model.Contracts.Messages;

namespace MGDF.GamesManager.Model.Contracts
{
    [ServiceContract(Name = "GamesManagerAdminService", Namespace = "http://schemas.matchstickframework.org/2010")]
    public interface IGamesManagerAdminService
    {
        [OperationContract]
        HearbeatResponse Heartbeat(HearbeatRequest request);

        [OperationContract]
        InstallGameResponse UpdateGame(UpdateGameRequest request);

        [OperationContract]
        InstallGameResponse InstallLocalGame(InstallLocalGameRequest request);

        [OperationContract]
        UninstallGameResponse UninstallGame(UninstallGameRequest request);

        [OperationContract]
        GetInstalledGameInfoResponse GetInstalledGameInfo(GetInstalledGameInfoRequest request);

        [OperationContract]
        ResumePendingOperationsResponse ResumePendingOperations(ResumePendingOperationsRequest request);

        [OperationContract]
        PausePendingOperationsResponse PausePendingOperations(PausePendingOperationsRequest request);

        [OperationContract]
        SubmitStatisticsResponse SubmitStatistics(SubmitStatisticsRequest request);
    }
}