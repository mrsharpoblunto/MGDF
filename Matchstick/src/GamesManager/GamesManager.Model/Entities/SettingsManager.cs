using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Common.Framework;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;

namespace MGDF.GamesManager.Model.Entities
{
    public class GameSettings
    {
        public string GameUid;
        public string UserName;
        public string Password;
        public bool? StatisticsServiceEnabled;
    }

    public class SettingsManager: JsonEntity
    {
        private const string SettingsFile = "GamesManagerSettings.json";

        private static SettingsManager _instance;

        private SettingsManager(string filename): base(filename)
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

        protected override void Load(JObject json)
        {
			var game = json.ReadToken("game");
            if (game!=null)
            {
                 var gameSource = new GameSettings
                 {
                     GameUid = game.ReadRequiredValue<string>("uid"),
                     UserName = game.ReadRequiredValue<string>("userName"),
                     Password = DPAPI.Decrypt(game.ReadRequiredValue<string>("passwordHash"))
                 };
                 game.ReadOptionalValue("statisticsServiceEnabled", ref gameSource.StatisticsServiceEnabled);
                 Settings = gameSource;
            }
        }

        public void Save()
        {
            using (var stream = FileSystem.Current.GetFile(FileSystem.Combine(Resources.GameUserDir, SettingsFile)).OpenStream(FileMode.Create))
            {
                using (var textWriter = new StreamWriter(stream))
                {
                    using (JsonWriter writer = new JsonTextWriter(textWriter))
                    {
                        writer.Formatting = Formatting.Indented;
                        writer.WriteStartObject();
                        writer.WritePropertyName("game");

                        writer.WriteStartObject();
                        writer.WriteRequiredValue("uid", Settings.GameUid);
                        if (Settings.StatisticsServiceEnabled.HasValue)
                        {
							writer.WritePropertyName("statisticsServiceEnabled");
                            writer.WriteValue(Settings.StatisticsServiceEnabled.Value);
                        }
                        writer.WriteRequiredValue("username", Settings.UserName ?? string.Empty);
                        writer.WriteRequiredValue("passwordHash",DPAPI.Encrypt(Settings.Password ?? string.Empty));

                        writer.WriteEndObject();

                        writer.WriteEndObject();
                    }
                }
            }
        }
    }
}