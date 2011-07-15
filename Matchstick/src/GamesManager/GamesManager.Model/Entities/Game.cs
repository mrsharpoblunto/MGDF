using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Mime;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Entities
{
    public class Game: XmlEntity
    {
        private byte[] _gameIconData;

        public static Game Current { get; set; }

        //when installed and loaded from the filesystem
        public Game(string gameFile)
            : base(gameFile, "game.xsd")
        {
            if (ErrorCollection.Count == 0)
            {
                LoadImageFromFileSystem();
            }
        }

        //when loaded from an install package
        public Game(IArchiveFile gameFile)
            : base(gameFile, "game.xsd")
        {
            if (ErrorCollection.Count == 0)
            {
                LoadImageFromArchive(gameFile);
            }
        }

        #region image loading methods

        private void LoadImageFromFileSystem()
        {
            if (FileSystem.Current.FileExists(Resources.GameIconFile()))
            {
                var file = FileSystem.Current.GetFile(Resources.GameIconFile());
                _gameIconData = file.ReadBinary();
            }
        }

        private void LoadImageFromArchive(IArchiveFile GameFile)
        {
            if (GameFile.Parent.Children.ContainsKey(Resources.GameIcon))
            {
                try
                {
                    IArchiveFile file = GameFile.Parent.Children[Resources.GameIcon];
                    using (var stream = file.OpenStream())
                    {
                        _gameIconData =new byte[stream.Length];
                        stream.Read(_gameIconData,0, (int)stream.Length);
                    }
                }
                catch (Exception)
                {
                    _gameIconData = null;
                }
            }
        }

        #endregion

        #region IGame Members

        public string Uid { get; private set; }
        public string Name { get; private set; }
        public Version Version { get; private set; }
        public int InterfaceVersion { get; private set; }
        public string Description { get; private set; }
        public string DeveloperUid { get; private set; }
        public string DeveloperName { get; private set; }

        public string Homepage { get; private set; }
        public string SupportEmail { get; private set; }

        public string GameSourceService { get; private set; }
        public string StatisticsService { get; private set; }
        public string StatisticsPrivacyPolicy { get; private set; }

        public byte[] GameIconData
        {
            get
            {
                return _gameIconData;
            }
        }

        #endregion

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                // parse based on NodeType
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        switch (reader.Name)
                        {
                            case "description":
                                Description = reader.ReadString();
                                break;

                            case "developeruid":
                                DeveloperUid = reader.ReadString();
                                break;

                            case "developername":
                                DeveloperName = reader.ReadString();
                                break;

                            case "homepage":
                                Homepage = reader.ReadString();
                                break;

                            case "version":
                                Version = new Version(reader.ReadString());
                                break;

                            case "gamename":
                                Name = reader.ReadString();
                                break;

                            case "gameuid":
                                Uid = reader.ReadString();
                                break;

                            case "gamesourceservice":
                                GameSourceService = reader.ReadString();
                                break;

                            case "statisticsservice":
                                StatisticsService = reader.ReadString();
                                break;

                            case "statisticsprivacypolicy":
                                StatisticsPrivacyPolicy = reader.ReadString();
                                break;

                            case "supportemail":
                                SupportEmail = reader.ReadString();
                                break;

                            case "interfaceversion":
                                InterfaceVersion = int.Parse(reader.ReadString());
                                break;
                        }
                        break;
                }
            }
            reader.Close();
        }

        public bool Equals(Game other)
        {
            return Uid == other.Uid && Version == other.Version;
        }

        public override int GetHashCode()
        {
            return (Uid + Version).GetHashCode();
        }

        public void Save(string filename)
        {
            IFile file = FileSystem.Current.GetFile(filename);

            //write out the game icon file along with the game definition file.
            if (GameIconData!=null)
            {
                var iconFile = FileSystem.Current.GetFile(Path.Combine(file.Parent.FullName,Resources.GameIcon));
                using (var stream = iconFile.OpenStream(FileMode.Create,FileAccess.Write,FileShare.None))
                {
                    stream.Write(GameIconData,0,GameIconData.Length);
                }
            }

            XmlWriterSettings settings = new XmlWriterSettings
                                             {
                                                 OmitXmlDeclaration = false,
                                                 Indent = true,
                                                 NewLineChars = "\r\n",
                                                 IndentChars = "\t"
                                             };

            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Create))
            {
                XmlWriter writer = XmlWriter.Create(stream, settings);

                writer.WriteStartDocument();
                writer.WriteStartElement("mgdf", "game", "http://schemas.matchstickframework.org/2007/game");
                writer.WriteAttributeString("xmlns", "xsi", null, "http://www.w3.org/2001/XMLSchema-instance");

                writer.WriteElementString("gameuid",Uid);
                writer.WriteElementString("gamename",Name);
                writer.WriteElementString("description",Description);
                writer.WriteElementString("version",Version.ToString());
                writer.WriteElementString("interfaceversion",InterfaceVersion.ToString());
                writer.WriteElementString("developeruid",DeveloperUid);
                writer.WriteElementString("developername", DeveloperName);

                writer.AddNonDefaultElementString("supportemail",SupportEmail);
                writer.AddNonDefaultElementString("homepage",Homepage);
                writer.AddNonDefaultElementString("gamesourceservice",GameSourceService);
                writer.AddNonDefaultElementString("statisticsservice",StatisticsService);
                writer.AddNonDefaultElementString("statisticsprivacypolicy",StatisticsPrivacyPolicy);

                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Close();
            }
        }

    }
}