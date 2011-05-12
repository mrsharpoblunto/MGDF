using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Caching;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.ServerCommon;
using Constants=MGDF.GamesManager.GameSource.Model.Constants;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public static class UploadManager
    {
        private static void WriteErrorResponse(int statusCode,string errorCode,string errorMessage,bool shouldRetry,IHttpResponse response)
        {
            response.Write("<error code=\"" + errorCode + "\" message=\"" + errorMessage + "\" retry=\"" + shouldRetry + "\" />");
            response.StatusCode = statusCode;
        }

        public static void ProcessRequest(IServerContext serverContext, IRepository repository, IHttpRequest request, IHttpResponse response)
        {
            try
            {
                response.ContentType = "text/xml";
                response.Write("<?xml version=\"1.0\" ?>");

                string developerKey = request.Headers[Common.Constants.Headers.DeveloperKey];
                string contentMd5 = request.Headers[Common.Constants.Headers.Md5];
                string cnonce = request.Headers[Common.Constants.Headers.CNonce];
                string hmac = request.Headers[Common.Constants.Headers.Hmac];
                string timestamp = request.Headers[Common.Constants.Headers.Timestamp];

                if (request.ContentLength>Config.Current.MaxUploadPartSize)
                {
                    WriteErrorResponse(400,Contracts.Messages.Error.InvalidArguments,"Part content length exceeds the maximum size (" + Config.Current.MaxUploadPartSize + " bytes)",false,response);
                    return;
                }

                if (string.IsNullOrEmpty(contentMd5))
                {
                    WriteErrorResponse(400, Contracts.Messages.Error.InvalidArguments, "Part content MD5 checksum header missing or invalid", false, response);
                    return;                    
                }

                try
                {
                    Guid parsedCnonce = new Guid(cnonce);

                    if (!CNonceCache.Instance.IsUnique(parsedCnonce, Common.Constants.AuthenticationTimeoutMinutes,repository))
                    {
                        WriteErrorResponse(401, Contracts.Messages.Error.AuthorizationFailed, "CNonce is not unique", false, response);
                        return;                       
                    }
                }
                catch (Exception)
                {
                    WriteErrorResponse(401, Contracts.Messages.Error.AuthorizationFailed, "Invalid CNonce format. Must be a valid Guid", false, response);
                    return;
                }

                //authenticate the developer
                Developer developer = repository.Get<Developer>().SingleOrDefault(u => u.DeveloperKey == developerKey);
                if (developer == null || !Cryptography.Authenticate(hmac, timestamp, cnonce + contentMd5, developer.SecretKey, Common.Constants.AuthenticationTimeoutMinutes))
                {
                    if (developer == null)
                    {
                        WriteErrorResponse(401, Contracts.Messages.Error.AuthorizationFailed, "No developer found with this Developer Key", false, response);
                    }
                    else
                    {
                        WriteErrorResponse(401, Contracts.Messages.Error.AuthorizationFailed, "Invalid HMac/Timestamp", false, response);
                    }
                    return;
                }

                //get the MGDF fragment data
                Guid fragmentId;
                bool isLastFragment;
                try
                {
                    fragmentId = new Guid(request.Headers[Common.Constants.Headers.FragmentIdHeader]);
                    isLastFragment = bool.Parse(request.Headers[Common.Constants.Headers.IsLastFragment]);
                }
                catch (Exception)
                {
                    WriteErrorResponse(400, Contracts.Messages.Error.InvalidArguments, "Expected MGDF headers were not present or are invalid", false, response);
                    return;
                }

                GameFragment fragment =
                    repository.Get<GameFragment>().SingleOrDefault(
                        f => f.Id == fragmentId && f.DeveloperId == developer.Id);

                if (fragment != null)
                {
                    try
                    {
                        if (!FileServer.Current.UploadPart(request, fragment, contentMd5, serverContext, repository))
                        {
                            WriteErrorResponse(400, Contracts.Messages.Error.InvalidArguments, "Invalid file part hash", true, response);
                            return;
                        }
                    }
                    catch (Exception)
                    {
                        WriteErrorResponse(500, Contracts.Messages.Error.UnknownError, "Unable to write to Game fragment", false, response);
                        return;
                    }

                    if (isLastFragment)
                    {
                        if (ValidateGameFragment(fragment, serverContext, repository))
                        {
                            //update the gameversion to point to the completed file, then delete the fragment.
                            GameVersion gameVersion = repository.Get<GameVersion>().Single(gv => gv.Id == fragment.GameVersionId);
                            gameVersion.GameDataId = FileServer.Current.CompleteUpload(fragment, serverContext, repository);
                            gameVersion.Md5Hash = fragment.Md5Hash;
                            gameVersion.Published = fragment.PublishOnComplete;
                            repository.Delete(fragment);
                            repository.SubmitChanges();

                            response.Write("<success gameVersionId=\"" + gameVersion.Id + "\" />");
                            response.StatusCode = 200;
                        }
                        else
                        {
                            WriteErrorResponse(400, Contracts.Messages.Error.InvalidArguments, "Invalid file hash", false, response);
                        }
                    }
                    else
                    {
                        response.Write("<success />");
                        response.StatusCode = 200;
                    }
                }
                else
                {
                    WriteErrorResponse(404, Contracts.Messages.Error.InvalidArguments, "No incomplete Game fragment with this id exists", false, response);
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unexpected exception in upload handler");
                try
                {
                    response.StatusCode = 500;
                }
                catch (Exception)
                {
                }
            }
        }

        private static bool ValidateGameFragment(GameFragment fragment,IServerContext serverContext,IRepository repository)
        {
            try
            {
                return FileServer.Current.ValidateCompletedFragment(fragment, serverContext, repository);
            }
            catch (Exception)
            {
                return false;
            }
        }
    }
}
