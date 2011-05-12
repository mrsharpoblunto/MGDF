using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.Text;

namespace GamesManager.Service.Contracts
{
    [ServiceContract(Name = "GamesManagerAdminService", Namespace = "http://schemas.matchstickframework.org/2010")]
    public interface IGamesManagerAdminService
    {
        [OperationContract]
        HearbeatResponse Heartbeat(HearbeatRequest request);

        [OperationContract]
        ShutDownResponse ShutDown(ShutDownRequest request);

        [OperationContract]
        InstallGameResponse InstallGame(InstallGameRequest request);

        [OperationContract]
        UninstallGameResponse UninstallGame(UninstallGameRequest request);

        [OperationContract]
        GetPendingOperationsResponse GetPendingOperations(GetPendingOperationsRequest request);

        [OperationContract]
        ResumePendingOperationResponse PausePendingOperations(ResumePendingOperationRequest request);

        [OperationContract]
        PausePendingOperationResponse PausePendingOperations(PausePendingOperationRequest request);

        [OperationContract]
        CancelPendingOperationResponse PausePendingOperations(CancelPendingOperationRequest request);
    }

    public class GetPendingOperationsRequest
    {
    }

    public class GetPendingOperationsResponse
    {
    }

    public class UninstallGameRequest
    {
        public string GameUid { get; set; }
    }

    public class UninstallGameResponse
    {
    }

    public class InstallGameRequest
    {
        public string UpdateGameUid { get; set; }
        public string InstallSource { get; set; }
        public bool LocalInstall { get; set; }
    }

    public class InstallGameResponse
    {
    }

    public class ShutDownRequest
    {
    }

    public class ShutDownResponse
    {
    }

    [DataContract]
    public class HearbeatRequest
    {
    }

    [DataContract]
    public class HearbeatResponse
    {
    }
}
