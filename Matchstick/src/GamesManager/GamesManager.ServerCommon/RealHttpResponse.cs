using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Web;

namespace MGDF.GamesManager.ServerCommon
{
    public interface IHttpResponse
    {
        int StatusCode { get; set; }
        NameValueCollection Headers { get; }
        string ContentType { get; set; }
        bool IsClientConnected { get; }
        void AppendHeader(string range, string s);
        void Flush();
        void WriteLine(string content);
        void WriteLine();
        void Write(byte[] buffer, int i, int read);
        void Write(string s);
        void SetCacheability(HttpCacheability cacheability);
        void End();
    }

    public class RealHttpResponse : IHttpResponse
    {
        private readonly HttpResponse _response;

        public RealHttpResponse(HttpResponse response)
        {
            _response = response;
        }

        public void End()
        {
            _response.End();
        }
        public int StatusCode
        {
            get { return _response.StatusCode; }
            set { _response.StatusCode = value; }
        }

        public NameValueCollection Headers
        {
            get { return _response.Headers; }
        }

        public string ContentType
        {
            get { return _response.ContentType; }
            set { _response.ContentType = value; }
        }

        public bool IsClientConnected
        {
            get { return _response.IsClientConnected; }
        }

        public void AppendHeader(string name, string value)
        {
            _response.AppendHeader(name, value);
        }

        public void Flush()
        {
            _response.Flush();
        }

        public void WriteLine(string content)
        {
            _response.Output.WriteLine(content);
        }

        public void WriteLine()
        {
            _response.Output.WriteLine();
        }

        public void Write(byte[] buffer, int i, int read)
        {
            _response.OutputStream.Write(buffer, i, read);
        }

        public void Write(string content)
        {
            _response.Write(content);
        }

        public void SetCacheability(HttpCacheability cacheability)
        {
            _response.Cache.SetCacheability(cacheability);
        }
    }
}