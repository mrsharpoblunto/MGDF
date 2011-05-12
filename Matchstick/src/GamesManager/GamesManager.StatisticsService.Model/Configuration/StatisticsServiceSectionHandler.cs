using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Xml;

namespace MGDF.GamesManager.StatisticsService.Model.Configuration
{
    public class StatisticsServiceSectionHandler : System.Configuration.IConfigurationSectionHandler
    {
        public object Create(object parent, object configContext, XmlNode section)
        {
            return new Config(section);
        }
    }
}