using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
using MGDF.GamesManager.Common;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace MGDF.GamesManager.Common.Framework
{
  public class GetCredentialsEventArgs : EventArgs
  {
    public string UserName { get; set; }
    public string Password { get; set; }
  }

  public interface IHttpRequestManager
  {
    TResponse GetJson<TResponse>(string uri);
    TResponse PostJson<TResponse, URequest>(string uri, URequest request);

    Stream Download(string uri, long progress, Func<GetCredentialsEventArgs, bool> getCredentials, out long contentLength);

    void Upload(string uri, Stream requestStream, string contentType);
  }

  public class HttpRequestManager : IHttpRequestManager
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

    public TResponse GetJson<TResponse>(string uri)
    {
      var request = (HttpWebRequest)WebRequest.Create(uri);
      request.Method = "GET";

      var response = (HttpWebResponse)request.GetResponse();
      using (var responseStream = response.GetResponseStream())
      {
        using (var reader = new StreamReader(responseStream))
        {

          return JsonConvert.DeserializeObject<TResponse>(reader.ReadToEnd(), new JsonSerializerSettings
          {
            ContractResolver = new CamelCasePropertyNamesContractResolver()
          });
        }
      }
    }

    public TResponse PostJson<TResponse, URequest>(string uri, URequest requestBody)
    {
      var request = (HttpWebRequest)WebRequest.Create(uri);
      request.Method = "POST";
      request.ContentType = "application/json";

      var serializerSettings = new JsonSerializerSettings
      {
        ContractResolver = new CamelCasePropertyNamesContractResolver()
      };

      var requestBodyContent = Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(requestBody, serializerSettings));
      using (var requestContentStream = request.GetRequestStream())
      {
        requestContentStream.Write(requestBodyContent, 0, requestBodyContent.Length);
      }

      var response = (HttpWebResponse)request.GetResponse();
      using (var responseStream = response.GetResponseStream())
      {
        using (var reader = new StreamReader(responseStream))
        {

          return JsonConvert.DeserializeObject<TResponse>(reader.ReadToEnd(), serializerSettings);
        }
      }
    }

    public Stream Download(string uri, long progress, Func<GetCredentialsEventArgs, bool> getCredentials, out long contentLength)
    {
      var credentials = new GetCredentialsEventArgs();
      bool requiresAuthentication = false;

      const int maxRedirects = 5;
      int redirects = 0;
      do
      {
        var request = (HttpWebRequest)WebRequest.Create(uri);
        request.Method = "GET";
        request.AllowAutoRedirect = false;

        if (requiresAuthentication)
        {
          var cache = new CredentialCache {
                        { new Uri(uri, UriKind.Absolute), "Basic", new NetworkCredential(credentials.UserName, credentials.Password) },
                        { new Uri(uri, UriKind.Absolute), "Digest", new NetworkCredential(credentials.UserName, credentials.Password) }
                    };
          request.Credentials = cache;
        }
        else
        {
          request.Credentials = CredentialCache.DefaultCredentials;
        }

        if (progress > 0) AddRange(progress, request);

        try
        {
          var response = (HttpWebResponse)request.GetResponse();

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

          if (ex.Response != null)
          {
            HttpWebResponse errorResponse = (HttpWebResponse)ex.Response;
            if (errorResponse.StatusCode == HttpStatusCode.Unauthorized && getCredentials != null && getCredentials(credentials))
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
      while (redirects++ < maxRedirects);

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

    public void Upload(string uri, Stream requestStream, string contentType)
    {
      var request = (HttpWebRequest)WebRequest.Create(uri);
      request.Method = "PUT";
      request.ContentType = contentType;

      using (var requestContentStream = request.GetRequestStream())
      {
        requestStream.CopyTo(requestContentStream);
      }
      request.GetResponse();
    }
  }
}
