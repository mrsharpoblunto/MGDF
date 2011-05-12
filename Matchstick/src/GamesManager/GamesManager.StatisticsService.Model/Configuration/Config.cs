using System;
using System.Xml;

namespace MGDF.GamesManager.StatisticsService.Model.Configuration
{
    public class Config : IConfig
    {
        private static IConfig _current;

        public static IConfig Current
        {
            get
            {
                if (_current == null)
                {
                    Config config = (Config)System.Configuration.ConfigurationManager.GetSection("statisticsService");
                    if (config == null)
                    {
                        throw new ApplicationException("Failed to get configuration from Web.config.");
                    }
                    _current = config;
                }
                return _current;
            }
            set
            {
                _current = value;
            }
        }

        private readonly XmlNode _section;

        public Config(XmlNode node)
        {
            _section = node;
        }

        private string this[string key]
        {
            get
            {
                XmlAttribute attr = _section.Attributes[key];
                if (attr != null)
                    return attr.Value;
                else
                    return null;
            }
        }

        public string ConnectionString
        {
            get { return this["connectionString"]; }
        }
    }
}