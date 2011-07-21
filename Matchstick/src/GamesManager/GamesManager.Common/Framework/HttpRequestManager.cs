using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Common.Framework
{
    public class GetCredentialsEventArgs : EventArgs
    {
        public string UserName { get; set; }
        public string Password { get; set; }
    }

    public interface IHttpRequestManager
    {
        Stream GetResponseStream(string uri);
        Stream GetResponseStream(string uri,long progress, Func<GetCredentialsEventArgs,bool> getCredentials, out long contentLength);
    }

    public class HttpRequestManager:IHttpRequestManager
    {
        public static IHttpRequestManager Current
        {
            get
            {
                return ServiceLocator.Current.Get<IHttpRequestManager>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }


        public Stream GetResponseStream(string uri)
        {
            long contentLength;
            return GetResponseStream(uri, 0, null,out contentLength);
        }

        public Stream GetResponseStream(string uri,long progress, Func<GetCredentialsEventArgs,bool> getCredentials,out long contentLength)
        {
            var credentials = new GetCredentialsEventArgs();
            bool requiresAuthentication = false;

            const int maxRedirects = 5;
            int redirects = 0;
            do
            {
                var request = (HttpWebRequest)WebRequest.Create(uri);
                request.AllowAutoRedirect = false;

                if (requiresAuthentication)
                {
                    var cache = new CredentialCache { 
                        { new Uri(uri, UriKind.Absolute), "Basic", new NetworkCredential(credentials.UserName, credentials.Password) },
                        { new Uri(uri, UriKind.Absolute), "Digest", new NetworkCredential(credentials.UserName, credentials.Password) } 
                    };
                    request.Credentials = cache;
                }

                if (progress>0) AddRange(progress, request);
                request.Credentials = CredentialCache.DefaultCredentials;

                try
                {
                    var response = (HttpWebResponse) request.GetResponse();

                    if (response.StatusCode == HttpStatusCode.Found)
                    {
                        uri = response.Headers["Location"];
                    }
                    else
                    {
                        contentLength = response.ContentLength;
                        return response.GetResponseStream();
                    }
                }
                catch (WebException ex)
                {
                    bool rethrow = true;

                    if (ex.Response!=null)
                    {
                        HttpWebResponse errorResponse = (HttpWebResponse) ex.Response;
                        if (errorResponse.StatusCode == HttpStatusCode.Unauthorized && getCredentials!=null && getCredentials(credentials))
                        {
                            //retry the request with the credentials supplied from the user, for all other errors
                            //just rethrow the exception.
                            requiresAuthentication = true;
                            rethrow = false;
                        }
                    }

                    if (rethrow) throw ex;
                }
            } 
            while (redirects++<maxRedirects);

            throw new Exception("Maximum redirect count exceeded");
        }

        //hack that gets around the standard framework method addRange only accepting an int32, 
        //hence limiting the max download size to 2GB
        private static void AddRange(long progress, HttpWebRequest request)
        {
            MethodInfo method = typeof(WebHeaderCollection).GetMethod
                ("AddWithoutValidate", BindingFlags.Instance | BindingFlags.NonPublic);
            string key = "Range";
            string val = string.Format("bytes={0}-", progress);
            method.Invoke(request.Headers, new object[] { key, val });
        }
    }
}
