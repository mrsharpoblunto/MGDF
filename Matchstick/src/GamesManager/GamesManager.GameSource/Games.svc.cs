using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.GameSource.Caching;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.Mapping;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using Developer=MGDF.GamesManager.GameSource.Model.Developer;
using Game=MGDF.GamesManager.GameSource.Model.Game;
using GameVersion=MGDF.GamesManager.GameSource.Model.GameVersion;

namespace MGDF.GamesManager.GameSource
{
    public class GamesService : CommonService, IGameSourceService
    {
        private delegate void MethodHandler<REQUEST, RESPONSE>(REQUEST request, RESPONSE response);
        private delegate bool ValidateRequestHandler<REQUEST>(REQUEST request, List<Error> errors);

        private static RESPONSE InvokeMethod<REQUEST, RESPONSE>(REQUEST request, MethodHandler<REQUEST, RESPONSE> handler, ValidateRequestHandler<REQUEST> requestValidator)
            where REQUEST : RequestBase
            where RESPONSE : ResponseBase, new()
        {
            RESPONSE response = new RESPONSE();

            try
            {
                if (requestValidator(request, response.Errors))
                {
                    handler(request, response);
                }
            }
            catch (Exception ex)
            {
                response.Errors.Add(new Error { Code = Error.UnknownError, Message = "Unknown error" });
                Logger.Current.Write(ex, "Error processing " + typeof(RESPONSE).Name);
            }

            return response;
        }

        private delegate void MethodHandler<REQUEST, RESPONSE,T>(REQUEST request, RESPONSE response,PassThroughContext<T> context);
        private delegate bool ValidateRequestHandler<REQUEST,T>(REQUEST request, PassThroughContext<T> context, List<Error> errors);

        private static RESPONSE InvokeMethod<REQUEST, RESPONSE,T>(REQUEST request, MethodHandler<REQUEST, RESPONSE,T> handler, ValidateRequestHandler<REQUEST,T> requestValidator)
            where REQUEST : RequestBase
            where RESPONSE : ResponseBase, new()
        {
            RESPONSE response = new RESPONSE();

            try
            {
                PassThroughContext<T> context = new PassThroughContext<T>();
                if (requestValidator(request, context, response.Errors))
                {
                    handler(request, response, context);
                }
            }
            catch (Exception ex)
            {
                response.Errors.Add(new Error { Code = Error.UnknownError, Message = "Unknown error" });
                Logger.Current.Write(ex, "Error processing " + typeof(RESPONSE).Name);
            }

            return response;
        }

        public GetGamesResponse GetGame(string interfaceVersion,string gameUid)
        {
            int version;
            int.TryParse(interfaceVersion, out version);
            GetGamesRequest request = new GetGamesRequest { InterfaceVersion = version, GameUid = gameUid };
            return InvokeMethod<GetGamesRequest, GetGamesResponse>(request, GetGamesHandler, (r,e) => true);
        }

        public GetGamesResponse GetGames(string developerUid, string interfaceVersion)
        {
            int version;
            int.TryParse(interfaceVersion, out version);
            GetGamesRequest request = new GetGamesRequest { InterfaceVersion = version, DeveloperUid = developerUid };
            return InvokeMethod<GetGamesRequest, GetGamesResponse>(request, GetGamesHandler, (r, e) => true);
        }

        public GetGameUpdateResponse GetGameUpdate(string interfaceVersion, string gameUid, string installedVersion)
        {
            //interfaceVersion parameter unuseed at present
            GetGameUpdateReqest request = new GetGameUpdateReqest { GameUid = gameUid, InstalledVersion = installedVersion };
            return InvokeMethod<GetGameUpdateReqest, GetGameUpdateResponse,Game>(request, GetGameUpdateHandler, GetGameUpdateRequestValidator);
        }

        private static bool GetGameUpdateRequestValidator(GetGameUpdateReqest request, PassThroughContext<Game> context,List<Error> errors)
        {
            try
            {
                Version version = new Version(request.InstalledVersion);
            }
            catch (Exception)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "Invalid version supplied" });
                return false;
            }

            if (string.IsNullOrEmpty(request.GameUid))
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No Game Uid supplied"});
                return false;
            }

            context.Context = GameCache.Instance.Get(request.GameUid);
            if (context.Context ==null)
            {
                errors.Add(new Error { Code = Error.InvalidArguments, Message = "No game exists with the supplied Uid" });
                return false;
            }

            return true;
        }

        private static void GetGameUpdateHandler(GetGameUpdateReqest request, GetGameUpdateResponse response,PassThroughContext<Game> context)
        {
            Version requestVersion = new Version(request.InstalledVersion);
            response.LatestVersion = GameVersionCache.Instance.Get(new GameVersionCacheRequest{Game = context.Context,Version = requestVersion});
        }
    }
}
