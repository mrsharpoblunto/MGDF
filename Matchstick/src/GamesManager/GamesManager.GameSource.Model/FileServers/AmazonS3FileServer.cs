using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Web;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public class AmazonS3FileServer: IFileServer
    {
        public string CreateGameData(Developer developer, Game game, GameVersion version, IServerContext context,IRepository repository)
        {
            //initialize multipart upload.
            var gameData = new AmazonS3FileServerGameData {Id = Guid.NewGuid(),NextPartIndex = 1};

            var amzHeaders = new Dictionary<string, string>();
            amzHeaders.Add("x-amz-acl",game.RequiresAuthentication ? "private" : "public-read");
            amzHeaders.Add("x-amz-meta", game.Uid);
            HttpWebRequest request = GenerateAuthorizedRequest("POST", string.Empty, "application/x-mgdf-archive",AmazonS3Config.Current.BasePath + gameData.Id + ".mza", "uploads", amzHeaders);
            request.Headers.Set("Content-Disposition","attachment; filename=\"" + game.Name + " "+version.Version + ".mza\"");
            request.Headers.Set("Cache-Control", "no-cache");

            XmlDocument response = GetResponseContent(request);
            gameData.Key = "/" + response.DocumentElement.GetElementsByTagName("Key")[0].InnerText;
            gameData.UploadID = response.DocumentElement.GetElementsByTagName("UploadId")[0].InnerText;

            repository.Insert(gameData);
            return gameData.Id.ToString();
        }

        public void DeleteGameData(string gameDataId, IServerContext serverContext, IRepository repository)
        {
            if (!string.IsNullOrEmpty(gameDataId))
            {
                //delete object from s3
                Guid versionId = new Guid(gameDataId);
                AmazonS3FileServerGameData data = repository.Get<AmazonS3FileServerGameData>().SingleOrDefault(d => d.Id == versionId);

                if (data.NextPartIndex==-1)
                {
                    //completed uploads
                    HttpWebRequest amzRequest = GenerateAuthorizedRequest("DELETE", string.Empty, string.Empty, data.Key, string.Empty, null);
                    GetResponse(amzRequest);
                }
                else
                {
                    //partial uploads
                    HttpWebRequest amzRequest = GenerateAuthorizedRequest("DELETE", string.Empty, string.Empty, data.Key, "uploadId=" + data.UploadID, null);

                    try
                    {
                        GetResponse(amzRequest);
                    }
                    catch (WebException ex)
                    {
                        if (((HttpWebResponse)ex.Response).StatusCode == HttpStatusCode.NotFound)
                        {
                            throw new Exception("Unable to delete partial upload data from s3, perhaps it has already been deleted or has completed uploading.");
                        }
                        else
                        {
                            throw ex;
                        }
                    }

                    repository.DeleteAll(GameSourceRepository.Current.Get<AmazonS3UploadPart>().Where(p => p.GameDataId == data.Id));
                }
                repository.Delete(data);
            }
        }

        public bool UploadPart(IHttpRequest request, GameFragment fragment, string partMd5,IServerContext context, IRepository repository)
        {
            //upload a single piece to s3 and store the resultant ETag in teh database
            Guid versionId = new Guid(fragment.GameDataId);
            AmazonS3FileServerGameData data = repository.Get<AmazonS3FileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            HttpWebRequest amzRequest = GenerateAuthorizedRequest("PUT", string.Empty, string.Empty,data.Key, "partNumber=" + data.NextPartIndex + "&uploadId=" + data.UploadID, null);

            using (Stream stream = amzRequest.GetRequestStream())
            {
                byte[] buffer = new byte[Constants.WriteBufferSize];
                int bytesRead = request.Read(buffer, 0, Constants.WriteBufferSize);
                while (bytesRead > 0)
                {
                    stream.Write(buffer, 0, bytesRead);
                    bytesRead = request.Read(buffer, 0, Constants.WriteBufferSize);
                }
            }

            var response = GetResponse(amzRequest);

            AmazonS3UploadPart part = new AmazonS3UploadPart
                                          {
                                              Id = Guid.NewGuid(),
                                              GameDataId = data.Id,
                                              ETag = response.Headers["ETag"],
                                              PartIndex = data.NextPartIndex++
                                          };
            repository.Insert(part);
            repository.SubmitChanges();
            return true;
        }

        public string CompleteUpload(GameFragment fragment, IServerContext context, IRepository repository)
        {
            Guid versionId = new Guid(fragment.GameDataId);
            AmazonS3FileServerGameData data = repository.Get<AmazonS3FileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            //reassemble the uploaded pieces on s3
            HttpWebRequest amzRequest = GenerateAuthorizedRequest("POST", string.Empty, string.Empty,data.Key, "uploadId=" + data.UploadID, null);

            using (var writer = new StreamWriter(amzRequest.GetRequestStream()))
            {
                writer.WriteLine("<CompleteMultipartUpload>");
                var parts = repository.Get<AmazonS3UploadPart>().Where(p => p.GameDataId == data.Id).OrderBy(p => p.PartIndex);
                foreach (var part in parts)
                {
                    writer.WriteLine("<Part>");
                    writer.WriteLine("<PartNumber>" + part.PartIndex + "</PartNumber>");
                    writer.WriteLine("<ETag>" + part.ETag + "</ETag>");
                    writer.WriteLine("</Part>");
                }
                writer.WriteLine("</CompleteMultipartUpload>");
            }

            GetResponseContent(amzRequest);

            data.NextPartIndex = -1;
            repository.DeleteAll(repository.Get<AmazonS3UploadPart>().Where(p => p.GameDataId == data.Id));
            return fragment.GameDataId;
        }

        public bool ValidateCompletedFragment(GameFragment fragment, IServerContext context, IRepository repository)
        {
            //validation in s3 is done for each uploaded part, so this step isn't necessary.
            return true;
        }

        public void ServeResponse(IHttpResponse response, Game game, GameVersion version, IRepository repository)
        {
            response.StatusCode = 302;
            response.AppendHeader(Headers.Location, GetRedirectUrl(game, version, repository));
            response.End();
        }

        private static string GetRedirectUrl(Game game, GameVersion version, IRepository repository)
        {
            Guid versionId = new Guid(version.GameDataId);
            AmazonS3FileServerGameData data = repository.Get<AmazonS3FileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            //return signed url with expiry if requires auth, otherwise just the raw url.
            if (game.RequiresAuthentication)
            {
                DateTime expires = TimeService.Current.Now.AddMinutes(15);
                long secondsSinceEpoch = (long)(expires - DateTime.SpecifyKind(new DateTime(1970, 1, 1), DateTimeKind.Utc)).TotalSeconds;

                string signature = PercentEncodeRfc3986(GenerateSignature(data.Key, "GET", string.Empty, string.Empty, null, secondsSinceEpoch.ToString()));

                return "http://s3.amazonaws.com/" + AmazonS3Config.Current.BucketId + data.Key + "?AWSAccessKeyId=" + AmazonS3Config.Current.AWSKey + "&Signature=" + signature + "&Expires=" + secondsSinceEpoch;
            }
            else
            {
                return "http://s3.amazonaws.com/" + AmazonS3Config.Current.BucketId + data.Key;
            }
        }

        private static HttpWebRequest GenerateAuthorizedRequest(string verb, string contentMd5, string contentType, string path, string queryString, Dictionary<string, string> amazonHeaders)
        {
            string pathAndQuery = path + (!string.IsNullOrEmpty(queryString) ? ("?" + queryString) : string.Empty);
            HttpWebRequest request = (HttpWebRequest)HttpWebRequest.Create("http://s3.amazonaws.com/" + AmazonS3Config.Current.BucketId + pathAndQuery);
            request.Method = verb;
            if (!string.IsNullOrEmpty(contentType))
            {
                request.ContentType = contentType;
            }
            if (!string.IsNullOrEmpty(contentMd5))
            {
                request.Headers.Add("Content-Md5", contentMd5);
            }
            if (amazonHeaders != null)
            {
                foreach (var amzHeader in amazonHeaders)
                {
                    request.Headers.Add(amzHeader.Key, amzHeader.Value);
                }
            }
            else
            {
                amazonHeaders = new Dictionary<string, string>();
            }

            //can't set the date header on a webrequest </facepalm>.
            string date = TimeService.Current.Now.ToString("r");
            request.Headers.Add("x-amz-date", date);
            amazonHeaders.Add("x-amz-date", date);

            string signature = GenerateSignature(pathAndQuery, verb, contentType, contentMd5, amazonHeaders, string.Empty);
            request.Headers.Set("Authorization", "AWS " + AmazonS3Config.Current.AWSKey + ":" + signature);

            return request;
        }

        private static string GenerateSignature(string path, string verb, string contentType, string contentMd5, IDictionary<string, string> amazonHeaders, string date)
        {
            string rawAuthorization = verb + "\n" + contentMd5 + "\n" + contentType + "\n" + date + "\n";

            if (amazonHeaders != null)
            {
                var sortedAmzHeaders = new SortedDictionary<string, string>(amazonHeaders);
                foreach (var header in sortedAmzHeaders)
                {
                    rawAuthorization += header.Key.ToLowerInvariant() + ":" + header.Value + "\n";
                }
            }
            rawAuthorization += "/" + AmazonS3Config.Current.BucketId + path;

            var hmac = new HMACSHA1(Encoding.UTF8.GetBytes(AmazonS3Config.Current.AWSSecretKey));
            byte[] toSign = Encoding.UTF8.GetBytes(rawAuthorization);

            byte[] sigBytes = hmac.ComputeHash(toSign);

            return Convert.ToBase64String(sigBytes);
        }

        private static XmlDocument GetResponseContent(HttpWebRequest request)
        {
            try
            {
                HttpWebResponse response = (HttpWebResponse)request.GetResponse();
                XmlDocument document = new XmlDocument();
                using (var reader = new StreamReader(response.GetResponseStream()))
                {
                    document.Load(reader);
                }

                if (document.DocumentElement.Name == "Error")
                {
                    string message = "S3 Error - Code: " + document.DocumentElement.GetElementsByTagName("Code")[0].Value + " " + document.DocumentElement.GetElementsByTagName("Message")[0].Value + " - " + document.DocumentElement.GetElementsByTagName("Resource")[0].Value;
                    Logger.Current.Write(LogInfoLevel.Error, message);
                    throw new Exception(message);
                }
                return document;
            }
            catch (WebException ex)
            {
                var response = (HttpWebResponse)ex.Response;
                XmlDocument document = new XmlDocument();
                using (var reader = new StreamReader(response.GetResponseStream()))
                {
                    document.Load(reader);
                }
                if (document.DocumentElement.Name == "Error")
                {
                    string message = "S3 Error - Code: " + document.DocumentElement.GetElementsByTagName("Code")[0].Value + " " + document.DocumentElement.GetElementsByTagName("Message")[0].Value + " - " + document.DocumentElement.GetElementsByTagName("Resource")[0].Value;
                    Logger.Current.Write(LogInfoLevel.Error, message);
                }
                throw ex;
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unexpected S3 Error");
                throw ex;
            }
        }

        private static HttpWebResponse GetResponse(HttpWebRequest request)
        {
            try
            {
                return (HttpWebResponse)request.GetResponse();
            }
            catch (WebException ex)
            {
                var response = (HttpWebResponse)ex.Response;
                Logger.Current.Write(ex, "Unexpected S3 Error: "+ response.StatusCode);
                throw ex;
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unexpected S3 Error");
                throw ex;
            }
        }

        private static string PercentEncodeRfc3986(string str)
        {
            str = HttpUtility.UrlEncode(str);
            str.Replace("'", "%27").Replace("(", "%28").Replace(")", "%29").Replace("*", "%2A").Replace("!", "%21").Replace("%7e", "~").Replace("+", "%20");

            StringBuilder sbuilder = new StringBuilder(str);
            for (int i = 0; i < sbuilder.Length; i++)
            {
                if (sbuilder[i] == '%')
                {
                    if (Char.IsDigit(sbuilder[i + 1]) && Char.IsLetter(sbuilder[i + 2]))
                    {
                        sbuilder[i + 2] = Char.ToUpper(sbuilder[i + 2]);
                    }
                }
            }
            return sbuilder.ToString();
        }
    }
}