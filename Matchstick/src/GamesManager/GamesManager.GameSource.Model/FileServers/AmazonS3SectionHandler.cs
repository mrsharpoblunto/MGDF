using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public class AmazonS3SectionHandler : System.Configuration.IConfigurationSectionHandler
    {
        public object Create(object parent, object configContext, XmlNode section)
        {
            return new AmazonS3Config(section);
        }
    }
}