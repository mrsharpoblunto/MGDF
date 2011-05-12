using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockHttpRequest: IHttpRequest
    {
        private readonly NameValueCollection _headers  = new NameValueCollection();
        private Stream _input;

        public string Path
        {
            get; set;
        }

        public NameValueCollection Headers
        {
            get { return _headers; }
        }

        public Uri Url
        {
            get; set;
        }

        public string HttpMethod
        {
            get; set;
        }

        public string UserHostAddress
        {
            get { return "127.0.0.1"; }
        }

        public int ContentLength
        {
            get { return _input!=null ? (int)_input.Length : 0; }
        }

        public int Read(byte[] buffer, int i, int size)
        {
            return _input.Read(buffer,i,size);
        }

        public byte[] Input
        {
            set { _input = new MemoryStream(value); }
        }
    }
}