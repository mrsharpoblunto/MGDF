using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Text;
using System.Web;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockHttpResponse: IHttpResponse
    {
        public event Func<bool> SetConnectionStatus;

        private readonly NameValueCollection _headers = new NameValueCollection();
        private readonly Stream _output = new MemoryStream();
        private readonly TextWriter _writer;
        private bool _responseEnded = false;

        public MockHttpResponse()
        {
            _writer = new StreamWriter(_output);
        }

        private HttpCacheability _cacheability;

        public int StatusCode
        {
            get; set;
        }

        public NameValueCollection Headers
        {
            get { return _headers; }
        }

        public string ContentType
        {
            get;
            set;
        }

        private bool _isClientConnected;

        public bool IsClientConnected
        {
            get
            {
                if (SetConnectionStatus!=null)
                {
                    _isClientConnected = SetConnectionStatus();
                }
                return _isClientConnected;
            }
            set { _isClientConnected = value; }
        }

        public void AppendHeader(string name, string value)
        {
            _headers.Add(name,value);
        }

        public void Flush()
        {
        }

        public void WriteLine(string content)
        {
            _writer.WriteLine(content);
            _writer.Flush();
        }

        public void WriteLine()
        {
            _writer.WriteLine();
            _writer.Flush();
        }

        public void Write(byte[] buffer, int i, int read)
        {
            _output.Write(buffer, i, read);
        }

        public void Write(string content)
        {
            _writer.Write(content);
            _writer.Flush();
        }

        public void SetCacheability(HttpCacheability cacheability)
        {
            _cacheability = cacheability;
        }

        public void End()
        {
            _responseEnded = true;
        }

        public bool ResponseEnded { get { return _responseEnded; } }

        public HttpCacheability Cacheability
        {
            get
            {
                return _cacheability;
            }
        }

        public Stream Output
        {
            get
            {
                return _output;
            }
        }
    }
}