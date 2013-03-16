using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Mime;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MGDF.GamesManager.Model.Entities
{
    public class Game: JsonEntity
    {
        private byte[] _gameIconData;

        public static Game Current { get; set; }

        //when installed and loaded from the filesystem
        public Game(string gameFile)
            : base(gameFile)
        {
        }

        //when loaded from an install package
        public Game(IArchiveFile gameFile)
            : base(gameFile)
        {
            if (ErrorCollection.Count == 0)
            {
                LoadImageFromArchive(gameFile);
            }
        }

        #region image loading methods

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
        public string DeveloperName { get; private set; }

        public string Homepage { get; private set; }
        public string SupportEmail { get; private set; }

        public string UpdateService { get; private set; }
        public string StatisticsService { get; private set; }
        public string StatisticsPrivacyPolicy { get; private set; }

        public byte[] GameIconData
        {
            get
            {
                if (_gameIconData == null)
                {
                    var file = FileSystem.Current.GetFile(Resources.GameIconFile());
                    if (file.Exists)
                    {
                        _gameIconData = file.ReadBinary();
                    }
                }
                return _gameIconData;
            }
        }

        #endregion

        protected override void Load(JObject json)
        {
            Description = json.ReadRequiredValue("description");
            DeveloperName = json.ReadRequiredValue("developername");
            Version = new Version(json.ReadRequiredValue("version"));
            Name = json.ReadRequiredValue("gamename");
            Uid = json.ReadRequiredValue("gameuid");
            InterfaceVersion = int.Parse(json.ReadRequiredValue("interfaceversion"));

            Homepage = json.ReadOptionalValue("homepage");
            UpdateService = json.ReadOptionalValue("updateservice");
            StatisticsService = json.ReadOptionalValue("statisticsservice");
            StatisticsPrivacyPolicy = json.ReadOptionalValue("statisticsprivacypolicy");
            SupportEmail = json.ReadOptionalValue("supportemail");
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

            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Create))
            {
                using (var textWriter = new StreamWriter(stream))
                {
                    using (JsonWriter writer = new JsonTextWriter(textWriter))
                    {
                        writer.Formatting = Formatting.Indented;
                        writer.WriteStartObject();

                        writer.WriteRequiredValue("gameuid",Uid);
                        writer.WriteRequiredValue("gamename",Name);
                        writer.WriteRequiredValue("description",Description);
                        writer.WriteRequiredValue("version",Version.ToString());
                        writer.WriteRequiredValue("interfaceversion",InterfaceVersion.ToString());
                        writer.WriteRequiredValue("developername",DeveloperName);
                        writer.WriteOptionalValue("supportemail", SupportEmail);
                        writer.WriteOptionalValue("homepage", Homepage);
                        writer.WriteOptionalValue("updateservice", UpdateService);
                        writer.WriteOptionalValue("statisticsservice", StatisticsService);
                        writer.WriteOptionalValue("statisticsprivacypolicy", StatisticsPrivacyPolicy);

                        writer.WriteEndObject();
                    }
                }
            }
        }

    }
}