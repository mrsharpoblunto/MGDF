using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class Response
  {
    public EventHandler OnRead;
    public EventHandler OnWrite;
    public byte[] Data;
    public int Status;
  }

  public class MockHttpRequestManager : IHttpRequestManager
  {
    private readonly Dictionary<string, Response> _responses = new Dictionary<string, Response>();
    private readonly Dictionary<string, KeyValuePair<string, string>> _credentials = new Dictionary<string, KeyValuePair<string, string>>();

    public void ExpectResponse(string uri, byte[] content)
    {
      ExpectResponse(uri, content, null, null);
    }

    public void SetCredentials(string uri, string username, string password)
    {
      _credentials.Add(uri, new KeyValuePair<string, string>(username, password));
    }

    public void ExpectResponse(string uri, string content)
    {
      ExpectResponse(uri, Encoding.UTF8.GetBytes(content), null, null);
    }

    public void ExpectResponse(string uri, byte[] content, EventHandler read, EventHandler write)
    {
      var response = new Response
      {
        Data = content,
        OnRead = read,
        OnWrite = write
      };
      if (_responses.ContainsKey(uri))
      {
        _responses[uri] = response;
      }
      else
      {
        _responses.Add(uri, response);
      }
    }

    public Stream GetResponseStream(string uri)
    {
      long contentLength;
      return GetResponseStream(uri, 0, out contentLength);
    }

    public Stream GetResponseStream(string uri, long progress, Func<GetCredentialsEventArgs, bool> getCredentials, out long contentLength)
    {
      var response = _responses[uri];

      if (response == null)
      {
        throw new WebException("404 File not found");
      }

      if (_credentials.ContainsKey(uri))
      {
        bool hasValidCredentials = false;
        if (getCredentials != null)
        {
          var args = new GetCredentialsEventArgs();
          while (getCredentials(args))
          {
            if (_credentials[uri].Key == args.UserName && _credentials[uri].Value == args.Password)
            {
              hasValidCredentials = true;
              break;
            }
          }
        }

        if (!hasValidCredentials)
        {
          throw new WebException("401 Unauthorized");
        }
      }
      contentLength = response.Data.Length;
      var stream = new MockStream(response.Data);
      if (response.OnRead != null) stream.OnRead += response.OnRead;
      if (response.OnWrite != null) stream.OnWrite += response.OnWrite;
      stream.Open();
      stream.Seek(progress, SeekOrigin.Begin);
      return stream;
    }


    public Stream GetResponseStream(string uri, long progress, out long contentLength)
    {
      return GetResponseStream(uri, progress, null, out contentLength);
    }

    public void ClearResponses()
    {
      _responses.Clear();
    }
  }
}