using System.Collections.Generic;
using System.ServiceModel;
using System.ServiceModel.Web;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts
{
    [ServiceContract(Name = "GamesService")]
    public interface IGameSourceService
    {
        [OperationContract]
        [WebGet(ResponseFormat=WebMessageFormat.Json,BodyStyle = WebMessageBodyStyle.Bare,UriTemplate = "/games/{interfaceVersion}/{gameUid}")]
        GetGamesResponse GetGame(string interfaceVersion,string gameUid);

        [OperationContract]
        [WebGet(ResponseFormat = WebMessageFormat.Json, BodyStyle = WebMessageBodyStyle.Bare, UriTemplate = "/developers/{developerUid}/games/{interfaceVersion}")]
        GetGamesResponse GetGames(string developerUid,string interfaceVersion);

        [OperationContract]
        [WebGet(ResponseFormat = WebMessageFormat.Json, BodyStyle = WebMessageBodyStyle.Bare, UriTemplate = "/updates/{gameUid}/{installedVersion}")]
        GetGameUpdateResponse GetGameUpdate(string gameUid,string installedVersion);
    }
}
