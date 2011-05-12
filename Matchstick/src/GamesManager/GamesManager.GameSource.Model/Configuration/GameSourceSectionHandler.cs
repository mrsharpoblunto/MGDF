using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Xml;

namespace MGDF.GamesManager.GameSource.Model.Configuration
{
    public class GameSourceSectionHandler : System.Configuration.IConfigurationSectionHandler
    {
        public object Create(object parent, object configContext, XmlNode section)
        {
            return new Config(section);
        }
    }
}