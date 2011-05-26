using System;
using System.Xml;

namespace MGDF.GamesManager.GameSource.Model.Configuration
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
                    Config config = (Config)System.Configuration.ConfigurationManager.GetSection("gameSource");
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

        public string FragmentUploadHandler
        {
            get { return this["fragmentUploadHandler"]; }
        }

        public string ConnectionString
        {
            get { return this["connectionString"]; }
        }

        public string AuthenticationRealm
        {
            get { return this["authenticationRealm"]; }
        }

        public string Name
        {
            get { return this["name"]; }
        }

        public string Description
        {
            get { return this["description"]; }
        }

        public string BaseUrl
        {
            get { return this["baseUrl"]; }
        }

        public string FileServerAssembly
        {
            get { return this["fileServerAssembly"]; }
        }

        public string FileServerType
        {
            get { return this["fileServerType"]; }
        }

        public int MaxUploadPartSize
        {
            get { return int.Parse(this["maxUploadPartSize"]); }
        }
    }
}