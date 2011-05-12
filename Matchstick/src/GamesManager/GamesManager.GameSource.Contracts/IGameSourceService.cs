using System.Collections.Generic;
using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts
{
    [ServiceContract(Name = "GamesService",Namespace = "http://schemas.matcstickframework.org/2010")]
    public interface IGameSourceService
    {
        [OperationContract]
        GetGamesResponse GetGames(GetGamesRequest request);

        [OperationContract]
        GetGameUpdateResponse GetGameUpdate(GetGameUpdateReqest request);
    }
}
