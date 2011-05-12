using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Web;

namespace MGDF.GamesManager.ServerCommon
{
    public interface IHttpRequest
    {
        string Path { get; }
        NameValueCollection Headers { get; }
        Uri Url { get; }
        string HttpMethod { get; }
        string UserHostAddress { get; }
        int ContentLength { get; }
        int Read(byte[] buffer, int i, int size);
    }

    public class RealHttpRequest: IHttpRequest
    {
        private readonly HttpRequest _request;

        public RealHttpRequest(HttpRequest request)
        {
            _request = request;
        }

        public int ContentLength
        {
            get { return _request.ContentLength; }
        }

        public string Path
        {
            get { return _request.Path; }
        }

        public NameValueCollection Headers
        {
            get { return _request.Headers; }
        }

        public Uri Url
        {
            get { return _request.Url; }
        }

        public string HttpMethod
        {
            get { return _request.HttpMethod; }
        }

        public string UserHostAddress
        {
            get { return _request.UserHostAddress; }
        }

        public int Read(byte[] buffer, int i, int size)
        {
            return _request.InputStream.Read(buffer, i, size);
        }
    }
}