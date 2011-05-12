using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public static class DownloadAuthenticationHelper
    {
        public static string GenerateAuthenticateChallenge(IHttpRequest request, GameVersion game,bool stale)
        {
            return "Digest realm=\"" + Config.Current.AuthenticationRealm + "\", algorithm=MD5, qop=\"auth\",nonce=\"" + GenerateNonce(request, game) + "\", opaque=\"" + game.Id.ToByteArray().ConvertToBase16() + "\""+ (stale ? ", stale=true":string.Empty);
        }

        private static string GenerateNonce(IHttpRequest request,GameVersion game)
        {
            return Encoding.UTF8.GetBytes(Cryptography.GenerateTimestamp(TimeService.Current.Now) + "|" + request.UserHostAddress + "|" + game.Md5Hash).ConvertToBase16();
        }

        public static bool CheckAuthentication(IRepository repository,IHttpRequest request, Game game, GameVersion version,out bool stale)
        {
            if (request.Url.Query.Contains(Common.Constants.QueryString.Hmac))
            {
                try
                {
                    string hmac=string.Empty;
                    string timestamp=string.Empty;
                    string[] queryComponents = request.Url.Query.Substring(1).Split(new[]{'&'}, StringSplitOptions.RemoveEmptyEntries);

                    foreach (var pair in queryComponents)
                    {
                        int split = pair.IndexOf("=");
                        string key = pair.Substring(0, split);
                        string value = pair.Substring(split+1);

                        if (key == Common.Constants.QueryString.Hmac)
                        {
                            hmac = value;
                        }
                        else if (key == Common.Constants.QueryString.Timestamp)
                        {
                            timestamp = value;
                        }
                    }

                    Developer developer = repository.Get<Developer>().Single(d => d.Id == game.DeveloperId);

                    stale = false;
                    return Cryptography.Authenticate(hmac, timestamp,version.Id.ToString(), developer.SecretKey, 15);
                }
                catch (Exception)
                {
                    stale = false;
                    return false;
                }

            }
            else if (!string.IsNullOrEmpty(request.Headers["Authorization"]))
            {
                string rawAuthHeader = request.Headers["Authorization"].Trim();

                if (rawAuthHeader.StartsWith("Digest"))
                {
                    string[] authComponents = rawAuthHeader.Substring(6).Split(new[] {','});
                    string method = request.HttpMethod;
                    string username = string.Empty;
                    string realm =string.Empty;
                    string uri=string.Empty;
                    string nonce=string.Empty;
                    string nc=string.Empty;
                    string cnonce=string.Empty;
                    string qop=string.Empty;
                    string response=string.Empty;

                    foreach (var authComponent in authComponents)
                    {
                        string[] keyValue = authComponent.Split(new[] {'='});
                        string key = keyValue[0].Trim();
                        string value = keyValue[1].Trim().Replace("\"", string.Empty);

                        switch (key)
                        {
                            case "username":
                                username = value;
                                break;
                            case "realm":
                                realm = value;
                                break;
                            case "uri":
                                uri = value;
                                break;
                            case "nonce":
                                nonce = value;
                                break;
                            case "nc":
                                nc = value;
                                break;
                            case "cnonce":
                                cnonce = value;
                                break;
                            case "qop":
                                qop = value;
                                break;
                            case "response":
                                response = value;
                                break;
                        }
                    }

                    if (IsValidNonce(nonce,request,version,out stale))
                    {
                        //get the user
                        User user = repository.Get<User>().SingleOrDefault(u => u.Name == username);
                        
                        if (user != null)
                        {
                            //does this user have access to this game
                            UserGame userGame = repository.Get<UserGame>().SingleOrDefault(ug => ug.UserId == user.Id && ug.GameId == game.Id);
                            if (userGame != null)
                            {
                                //now verify that the password supplied in the digest result is correct
                                return CheckAuthentication(user, realm, method, uri, nonce, nc, cnonce, qop, response);
                            }
                        }
                    }
                }
            }
            stale = false;
            return false;
        }

        /// <summary>
        /// valid nonce values must be generated within the last 15 minutes to prevent replay attacks.
        /// They also have to include the md5 hash of the correct game.
        /// </summary>
        /// <param name="nonce"></param>
        /// <param name="game"></param>
        /// <returns></returns>
        private static bool IsValidNonce(string nonce, IHttpRequest request,GameVersion version,out bool stale)
        {
            try
            {
                string convertedNonce = Encoding.UTF8.GetString(nonce.ConvertFromBase16());
                string[] nonceComponents = convertedNonce.Split(new[] {'|'});
                DateTime nonceTimeStamp = DateTime.ParseExact(nonceComponents[0],"yyyy-MM-ddTHH:mm:ssZ", CultureInfo.InvariantCulture,DateTimeStyles.AssumeUniversal).ToUniversalTime();
                string remoteIp = nonceComponents[1];
                string nonceMd5 = nonceComponents[2];

                TimeSpan diff = new TimeSpan(Math.Abs(TimeService.Current.Now.Ticks - nonceTimeStamp.Ticks));
                stale = diff.TotalMinutes > 1;
                return remoteIp == request.UserHostAddress && nonceMd5 == version.Md5Hash;
            }
            catch (Exception)
            {
                stale = false;
                return false;
            }
        }

        private static bool CheckAuthentication(User user, string realm, string method, string uri, string nonce, string nc, string cnonce, string qop, string response)
        {
            using (HashAlgorithm hashAlg = MD5.Create())
            {
                if (realm != Config.Current.AuthenticationRealm)
                {
                    throw new Exception("Incorrect authentication realm");
                }

                string ha1 = user.PasswordHash;

                string a2 = method + ":" + uri;
                string ha2 = a2.ComputeHash(hashAlg);

                string generatedResponse = ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2;
                string hGeneratedResponse = generatedResponse.ComputeHash(hashAlg);

                //if the client and server generated vlaues agree, then the user is authenticated
                return hGeneratedResponse == response;
            }
        }
    }
}
