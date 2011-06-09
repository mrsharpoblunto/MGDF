using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using Newtonsoft.Json;

namespace MGDF.GamesManager.Model.Helpers
{
    public class GameSourceServices
    {
        public string GameSourceService;
        public string DeveloperService;
    }

    public class GameSourceServiceLocator
    {
        private readonly GameSourceServices _services;
        private readonly string _baseUrl;

        public string GamesServiceUrl
        {
            get
            {
                return _services.GameSourceService.AbsoluteUri(_baseUrl);
            }
        }

        public string DeveloperServiceUrl
        {
            get
            {
                return _services.GameSourceService.AbsoluteUri(_baseUrl);
            }
        }

        public GameSourceServiceLocator(string baseUrl)
        {
            _services = new GameSourceServices
            {
                GameSourceService = "/Games.svc",
                DeveloperService = "/Developer.svc"
            };

            _baseUrl = baseUrl;

            try
            {

                using (var responseStream = HttpRequestManager.Current.GetResponseStream("/services".AbsoluteUri(baseUrl)))
                {
                    using (TextReader reader = new StreamReader(responseStream))
                    {
                        _services = JsonConvert.DeserializeObject<GameSourceServices>(reader.ReadToEnd());
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(LogInfoLevel.Error,"Unable to locate services: " + ex);
                throw ex;
            }
        } 
    }
}