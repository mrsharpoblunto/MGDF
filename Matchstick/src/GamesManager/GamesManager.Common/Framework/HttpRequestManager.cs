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
    public interface IHttpRequestManager
    {
        Stream GetResponseStream(string uri);
        Stream GetResponseStream(string uri,long progress, out long contentLength);

        bool HasValidCredentials(string uri, string username, string password);
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
            return GetResponseStream(uri, 0, out contentLength);
        }

        public Stream GetResponseStream(string uri,long progress, out long contentLength)
        {
            const int maxRedirects = 5;
            int redirects = 0;
            do
            {
                var request = (HttpWebRequest)WebRequest.Create(uri);
                request.AllowAutoRedirect = false;

                if (progress>0) AddRange(progress, request);
                request.Credentials = CredentialCache.DefaultCredentials;

                var response = (HttpWebResponse)request.GetResponse();

                if (response.StatusCode==HttpStatusCode.Found)
                {
                    uri = response.Headers["Location"];
                }
                else
                {
                    contentLength = response.ContentLength;
                    return response.GetResponseStream();
                }
            } 
            while (redirects++<maxRedirects);

            throw new Exception("Maximum redirect count exceeded");
        }

        public bool HasValidCredentials(string uri, string username, string password)
        {
            var cache = new CredentialCache{{new Uri(uri, UriKind.Absolute), "Digest", new NetworkCredential(username, password)}};
            var request = (HttpWebRequest)WebRequest.Create(uri);
            request.Credentials = cache;

            try
            {
                using (var response = (HttpWebResponse)request.GetResponse())
                {
                    return response.StatusCode != HttpStatusCode.Unauthorized;
                }
            }
            catch (WebException ex)
            {
                using (var response = (HttpWebResponse)ex.Response)
                {
                    return response.StatusCode != HttpStatusCode.Unauthorized;
                }
            }
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
