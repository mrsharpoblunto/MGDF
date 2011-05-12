using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.Text;
using System.Xml.Serialization;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using File=System.IO.File;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Model
{
    [XmlRoot("Settings")]
    public class Settings
    {
        private const string SettingsFile = "DataLoaderSettings.xml";

        private static Settings _instance;
        private string _developerKey;
        private string _secretKey;
        private string _gameSource;

        public static Settings Instance
        {
            get
            {
                if (_instance == null)
                {
                    if (FileSystem.Current.FileExists(string.Format("{0}/{1}", EnvironmentSettings.Current.UserDirectory, SettingsFile)))
                    {
                        XmlSerializer serializer = new XmlSerializer(typeof(Settings));
                        var file = FileSystem.Current.GetFile(string.Format("{0}/{1}", EnvironmentSettings.Current.UserDirectory, SettingsFile));
                        using (var stream = file.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
                        {
                            using (var reader = new StreamReader(stream))
                            {
                                _instance = (Settings) serializer.Deserialize(reader);
                            }
                        }
                        _instance.SecretKey = DPAPI.Decrypt(_instance.SecretKey);//decrypt the password after loading
                    }
                    else
                    {
                        _instance = new Settings();
                    }
                }
                return _instance;
            }
            set
            {
                _instance = value;
            }
        }

        [XmlElement("DeveloperKey")]
        public string DeveloperKey
        {
            get { return _developerKey; }
            set { _developerKey = value; }
        }

        [XmlElement("GameSource")]
        public string GameSource
        {
            get { return _gameSource; }
            set { _gameSource = value; }
        }

        [XmlElement("SecretKey")]
        public string SecretKey
        {
            get { return _secretKey; }
            set { _secretKey = value; }
        }

        [XmlIgnore]
        public Developer Credentials
        {
            get; set;
        }

        public void Save()
        {
            XmlSerializer serializer = new XmlSerializer(typeof(Settings));
            var file = FileSystem.Current.GetFile(string.Format("{0}/{1}", EnvironmentSettings.Current.UserDirectory, SettingsFile));
            using (var stream = file.OpenStream(FileMode.Create, FileAccess.Write, FileShare.None))
            {
                using (var writer = new StreamWriter(stream))
                {
                    _secretKey = DPAPI.Encrypt(_secretKey);
                    serializer.Serialize(writer, this);
                    _secretKey = DPAPI.Decrypt(_secretKey);
                }
            }
        }

        public void Clear()
        {
            var file = FileSystem.Current.GetFile(string.Format("{0}/{1}", EnvironmentSettings.Current.UserDirectory, SettingsFile));

            if (file.Exists)
            {
                file.Delete();
            }
        }
    }
}
