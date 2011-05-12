using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.Model.Entities.XmlEntities
{
    public class GameSettings
    {
        public string GameUid { get; set; }
        public string UserName { get; set; }
        public string Password { get; set; }
        public bool? StatisticsServiceEnabled { get; set; }
    }

    public class SettingsManager: XmlEntity, IDisposable
    {
        private const string SettingsFile = "GamesManagerSettings.xml";

        private readonly List<GameSettings> _games = new List<GameSettings>();
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
                    if (FileSystem.Current.FileExists(FileSystem.Combine(EnvironmentSettings.Current.UserDirectory, SettingsFile)))
                    {
                        _instance = new SettingsManager(FileSystem.Combine(EnvironmentSettings.Current.UserDirectory, SettingsFile));
                    }
                    else if (FileSystem.Current.FileExists(FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, SettingsFile)))
                    {
                        _instance = new SettingsManager(FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, SettingsFile));
                    }
                    else
                    {
                        _instance = new SettingsManager();
                    }
                }
                return _instance;
            }
        }

        public List<GameSettings> Games
        {
            get
            {
                return _games;
            }
        }

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
                            _games.Add(gameSource);
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

            using (var stream = FileSystem.Current.GetFile(FileSystem.Combine(EnvironmentSettings.Current.UserDirectory, SettingsFile)).OpenStream(FileMode.Create))
            {
                XmlWriter writer = XmlWriter.Create(stream, settings);

                writer.WriteStartDocument();
                writer.WriteStartElement("settings");
                foreach (GameSettings gameSetting in _games)
                {
                    writer.WriteStartElement("game");
                    writer.WriteAttributeString("uid", gameSetting.GameUid);
                    if (gameSetting.StatisticsServiceEnabled.HasValue)
                    {
                        writer.WriteAttributeString("statisticsserviceenabled", gameSetting.StatisticsServiceEnabled.ToString());
                    }
                    writer.WriteAttributeString("username", gameSetting.UserName ?? string.Empty);
                    writer.WriteAttributeString("passwordhash", DPAPI.Encrypt(gameSetting.Password ?? string.Empty));
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Close();
            }
        }

        public void Dispose()
        {
            _instance = null;
        }
    }
}