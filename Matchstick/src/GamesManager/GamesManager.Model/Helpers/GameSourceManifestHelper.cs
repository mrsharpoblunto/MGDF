using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Helpers
{
    public class GameSourceManifestHelper
    {
        private readonly string _relativeGamesServiceUrl;
        private readonly string _relativeDeveloperServiceUrl;
        private readonly string _baseUrl;

        public string GamesServiceUrl
        {
            get
            {
                return _relativeGamesServiceUrl.AbsoluteUri(_baseUrl);
            }
        }

        public string DeveloperServiceUrl
        {
            get
            {
                return _relativeDeveloperServiceUrl.AbsoluteUri(_baseUrl);
            }
        }

        public GameSourceManifestHelper(string baseUrl)
        {
            _relativeGamesServiceUrl = "/Games.svc";
            _relativeDeveloperServiceUrl = "/Developer.svc";
            _baseUrl = baseUrl;

            try
            {
                XmlDocument document = new XmlDocument();
                using (var responseStream = HttpRequestManager.Current.GetResponseStream("/gamesourcemanifest.xml".AbsoluteUri(baseUrl)))
                {
                    document.Load(responseStream);

                    var gamesService = document.GetElementsByTagName("gamesourceservice");
                    if (gamesService.Count == 0)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, "Unable to read gamesourceservice from gamesourcemanifest.xml, defaulting to Games.svc");
                    }
                    else
                    {
                        _relativeGamesServiceUrl = gamesService[0].InnerText;
                    }

                    var developerService = document.GetElementsByTagName("developerservice");
                    if (developerService.Count == 0)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, "Unable to read developerservice from gamesourcemanifest.xml, defaulting to Developer.svc");
                    }
                    else
                    {
                        _relativeDeveloperServiceUrl = developerService[0].InnerText;
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(LogInfoLevel.Error,"Unable to read gamesourcemanifest.xml: " + ex);
                throw ex;
            }
        } 
    }
}