using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public class AmazonS3Config
    {
        private static AmazonS3Config _current;

        public static AmazonS3Config Current
        {
            get
            {
                if (_current == null)
                {
                    AmazonS3Config config = (AmazonS3Config)System.Configuration.ConfigurationManager.GetSection("amazonS3");
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

        public AmazonS3Config(XmlNode node)
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

        public string BucketId
        {
            get { return this["bucketId"]; }
        }

        public string BasePath
        {
            get { return this["basePath"]; }
        }

        public string AWSKey
        {
            get { return this["awsKey"]; }
        }

        public string AWSSecretKey
        {
            get { return this["awsSecretKey"]; }
        }
    }
}