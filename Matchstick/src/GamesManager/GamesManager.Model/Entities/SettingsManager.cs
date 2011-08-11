using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Entities
{
    public class GameSettings
    {
        public string GameUid { get; set; }
        public string UserName { get; set; }
        public string Password { get; set; }
        public bool? StatisticsServiceEnabled { get; set; }
    }

    public class SettingsManager: XmlEntity
    {
        private const string SettingsFile = "GamesManagerSettings.xml";

        private static SettingsManager _instance;

        private SettingsManager(string filename): base(filename,null)
        {
        }

        private SettingsManager()
        {
        }

        public static SettingsManager Instance
        {
            get
            {
                if (_instance == null)
                {
                    if (FileSystem.Current.FileExists(FileSystem.Combine(Resources.GameUserDir, SettingsFile)))
                    {
                        _instance = new SettingsManager(FileSystem.Combine(Resources.GameUserDir, SettingsFile));
                    }
                    else
                    {
                        _instance = new SettingsManager();
                    }
                }
                return _instance;
            }
        }

        public static void Dispose()
        {
            _instance = null;
        }

        public GameSettings Settings { get; set; }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name.Equals("game"))
                        {
                            var gameSource = new GameSettings
                                                 {
                                                     GameUid = reader.GetAttribute("uid"),
                                                     UserName = reader.GetAttribute("username"),
                                                     Password = DPAPI.Decrypt(reader.GetAttribute("passwordhash"))
                                                 };
                            if (!string.IsNullOrEmpty(reader.GetAttribute("statisticsserviceenabled")))
                            {
                                gameSource.StatisticsServiceEnabled = bool.Parse(reader.GetAttribute("statisticsserviceenabled"));
                            }
                            Settings = gameSource;
                        }
                        break;
                }
            }
        }

        public void Save()
        {
            XmlWriterSettings settings = new XmlWriterSettings
                                             {
                                                 OmitXmlDeclaration = false,
                                                 Indent = true,
                                                 NewLineChars = "\r\n",
                                                 IndentChars = "\t"
                                             };

            using (var stream = FileSystem.Current.GetFile(FileSystem.Combine(Resources.GameUserDir, SettingsFile)).OpenStream(FileMode.Create))
            {
                XmlWriter writer = XmlWriter.Create(stream, settings);

                writer.WriteStartDocument();
                writer.WriteStartElement("settings");

                writer.WriteStartElement("game");
                writer.WriteAttributeString("uid", Settings.GameUid);
                if (Settings.StatisticsServiceEnabled.HasValue)
                {
                    writer.WriteAttributeString("statisticsserviceenabled", Settings.StatisticsServiceEnabled.ToString());
                }
                writer.WriteAttributeString("username", Settings.UserName ?? string.Empty);
                writer.WriteAttributeString("passwordhash", DPAPI.Encrypt(Settings.Password ?? string.Empty));
                writer.WriteEndElement();

                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Close();
            }
        }
    }
}