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
        [WebGet(ResponseFormat=WebMessageFormat.Json,BodyStyle = WebMessageBodyStyle.Bare,UriTemplate = "/downloads/{interfaceVersion}/{gameUid}/latest.json")]
        GetGamesResponse GetGame(string interfaceVersion,string gameUid);

        [OperationContract]
        [WebGet(ResponseFormat = WebMessageFormat.Json, BodyStyle = WebMessageBodyStyle.Bare, UriTemplate = "/Developers/{developerUid}/Games/{interfaceVersion}")]
        GetGamesResponse GetGames(string developerUid,string interfaceVersion);

        [OperationContract]
        [WebGet(ResponseFormat = WebMessageFormat.Json, BodyStyle = WebMessageBodyStyle.Bare, UriTemplate = "/downloads/{interfaceVersion}/{gameUid}/{installedVersion}.json")]
        GetGameUpdateResponse GetGameUpdate(string interfaceVersion,string gameUid,string installedVersion);
    }
}
