using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockConfig : IConfig
    {
        public string BaseUrl
        {
            get { return "http://games.junkship.org"; }
        }

        public string FragmentUploadHandler
        {
            get { return "/upload.axd"; }
        }

        public string ConnectionString
        {
            get { return string.Empty; }
        }

        public string AuthenticationRealm
        {
            get { return "example.com"; }
        }

        public string Name
        {
            get { return "gamesource"; }
        }

        public string Description
        {
            get { return "gamesource"; }
        }

        public string DownloadAuthenticationProviderAssembly
        {
            get { return string.Empty; }
        }

        public string DownloadAuthenticationProviderType
        {
            get { return string.Empty; }
        }

        public string DownloadAuthenticationProviderConnectionString
        {
            get { return string.Empty; }
        }

        public string FileServerAssembly
        {
            get { return string.Empty; }
        }

        public string FileServerType
        {
            get { return string.Empty; }
        }

        private int _maxUploadPartSize = 2097152;
        public int MaxUploadPartSize
        {
            get { return _maxUploadPartSize; }
            set
            {
                _maxUploadPartSize = value;
            }
        }
    }
}