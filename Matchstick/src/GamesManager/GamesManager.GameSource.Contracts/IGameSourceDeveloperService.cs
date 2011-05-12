using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts
{
    [ServiceContract(Name = "DeveloperService", Namespace = "http://schemas.matcstickframework.org/2010")]
    public interface IGameSourceDeveloperService: IGameSourceService
    {
        [OperationContract]
        CheckCredentialsResponse CheckCredentials(AuthenticatedRequestBase request);

        [OperationContract]
        AddGameVersionResponse AddGameVersion(AddGameVersionRequest versionRequest);

        [OperationContract]
        GetGameVersionsResponse GetGameVersions(GetGameVersionsRequest request);

        [OperationContract]
        DeleteGameVersionResponse DeleteGameVersion(DeleteGameVersionRequest request);

        [OperationContract]
        EditGameVersionResponse EditGameVersion(EditGameVersionRequest versionRequest);

        [OperationContract]
        CleanupIncompleteGameVersionResponse CleanupIncompleteGameVersion(CleanupIncompleteGameVersionRequest versionRequest);

        [OperationContract]
        AddGameResponse AddGame(AddGameRequest request);

        [OperationContract]
        EditGameResponse EditGame(EditGameRequest request);

        [OperationContract]
        DeleteGameResponse DeleteGame(DeleteGameRequest request);

        [OperationContract]
        GetOrCreateUserResponse GetOrCreateUser(GetOrCreateUserRequest request);//get an existing user or create one if no user exists

        [OperationContract]
        EditUserResponse EditUser(EditUserRequest request);//change password

        [OperationContract]
        GetUserGamesResponse GetUserGames(GetUserGamesRequest request);//get list of games for this dev the user has access to + temp secure download links for those games

        [OperationContract]
        EditUserGamesResponse EditUserGames(EditUserGamesRequest request);//give/revoke access to games (this dev only)
    }
}
