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
        public string DeveloperName { get; private set; }

        public string Homepage { get; private set; }
        public string SupportEmail { get; private set; }
		public string SupportS3Bucket { get; private set; }
		public string SupportS3BucketAccessKey { get; private set; }
		public string SupportS3BucketSecretKey { get; private set; }

        public string UpdateService { get; private set; }
        public string StatisticsService { get; private set; }
        public string StatisticsPrivacyPolicy { get; private set; }
		public Dictionary<string, string> Preferences { get; private set; }

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
            DeveloperName = json.ReadRequiredValue<string>("developerName");
            Version = new Version(json.ReadRequiredValue<string>("version"));
            Name = json.ReadRequiredValue<string>("gameName");
            Uid = json.ReadRequiredValue<string>("gameUid");
            InterfaceVersion = int.Parse(json.ReadRequiredValue<string>("interfaceVersion"));

            Homepage = json.ReadOptionalValue<string>("homepage");
            UpdateService = json.ReadOptionalValue<string>("updateService");
            StatisticsService = json.ReadOptionalValue<string>("statisticsService");
            StatisticsPrivacyPolicy = json.ReadOptionalValue<string>("statisticsPrivacyPolicy");
            SupportEmail = json.ReadOptionalValue<string>("supportEmail");
			SupportS3Bucket = json.ReadOptionalValue<string>("supportS3Bucket");
			SupportS3BucketAccessKey = json.ReadOptionalValue<string>("supportS3BucketAccessKey");
			SupportS3BucketSecretKey = json.ReadOptionalValue<string>("supportS3BucketSecretKey");

			Preferences = new Dictionary<string, string>();
			if (json.ReadToken("preferences") != null)
			{
				foreach (JProperty pref in json.ReadToken("preferences"))
				{
					Preferences.Add(pref.Name, pref.Value.ToString());
				}
			}
        }

        public bool Equals(Game other)
        {
            return Uid == other.Uid && Version == other.Version;
        }

        public override int GetHashCode()
        {
            return (Uid + Version).GetHashCode();
        }
    }
}