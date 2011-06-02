using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Caching;
using MGDF.GamesManager.GameSource.Mapping;
using MGDF.GamesManager.GameSource.Model;
using System.Net;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.ServerCommon;
using Constants=MGDF.GamesManager.GameSource.Model.Constants;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public static class DownloadManager
    {
        public static void ProcessRequest(IServerContext serverContext,IRepository repository,IHttpRequest request, IHttpResponse response)
        {
            try
            {
                //disallow access to .fragment files.
                if (request.Path.ToLowerInvariant().Contains(".fragment"))
                {
                    response.StatusCode = 404;
                    response.End();
                    return;
                }

                string[] requestComponents = request.Path.Split(new[] { '/' }, StringSplitOptions.RemoveEmptyEntries);

                if (requestComponents.Length == 3 && request.Path.EndsWith("/latestVersion", StringComparison.InvariantCultureIgnoreCase))
                {
                    //rewrite requests to /games/uid/latestVersion to the latest actual version.
                    string gameUid = requestComponents[requestComponents.Length - 2];
                    Game game = repository.Get<Game>().SingleOrDefault(g => g.Uid == gameUid);
                    if (game!=null)
                    {
                        GameVersion latestVersion = GameMapper.GetLatestVersion(game,repository);
                        if (latestVersion != null)
                        {
                            response.StatusCode = 302;
                            response.AppendHeader(Headers.Location, Config.Current.BaseUrl + "/games/" + latestVersion.Id.Encode() + ".mza");
                            response.End();
                            return;
                        }
                    }
                    response.StatusCode = 404;
                    response.End();
                }
                else
                {
                    GameVersion gameVersion=null;
                    if (requestComponents.Length == 2)
                    {
                        try
                        {
                            string rawVersionId = requestComponents[requestComponents.Length - 1];
                            if (rawVersionId.EndsWith(".mza", StringComparison.InvariantCultureIgnoreCase))
                            {
                                rawVersionId = rawVersionId.Substring(0, rawVersionId.Length - 4);
                            }
                            Guid gameVersionId = rawVersionId.Decode();
                            gameVersion = repository.Get<GameVersion>().SingleOrDefault(g => g.Id == gameVersionId && g.Published);
                        }
                        catch (Exception ex)
                        {
                            Logger.Current.Write(ex, "Invalid game file specified " + request.Path);
                            gameVersion = null;
                        }
                    }

                    Game game = gameVersion != null ? repository.Get<Game>().Single(g => g.Id == gameVersion.GameId) : null;
                    bool stale = false;

                    if (gameVersion == null)
                    {
                        response.StatusCode = 404;
                        response.End();
                    }
                    else if (game.RequiresAuthentication && !DownloadAuthenticationHelper.CheckAuthentication(repository, request, game, gameVersion, out stale))
                    {
                        response.StatusCode = 401;
                        response.Headers.Add(Headers.WWWAuthenticate, DownloadAuthenticationHelper.GenerateAuthenticateChallenge(request, gameVersion, false));
                        response.End();
                    }
                    else if (stale)//auth passed, but the nonce is stale
                    {
                        response.StatusCode = 401;
                        response.Headers.Add(Headers.WWWAuthenticate, DownloadAuthenticationHelper.GenerateAuthenticateChallenge(request, gameVersion, true));
                        response.End();
                    }
                    else
                    {
                        FileServer.Current.ServeResponse(response, game, gameVersion, repository);
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unexpected exception in download manager");
                try
                {
                    response.StatusCode = 500;
                    response.End();
                }
                catch (Exception)
                {
                }
            }
        } 
    }
}