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
        public string GameUid { get; set; }
        public string UserName { get; set; }
        public string Password { get; set; }
        public bool? StatisticsServiceEnabled { get; set; }
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
            var game = json["game"];
            if (game!=null)
            {
                 var gameSource = new GameSettings
                 {
                     GameUid = game.ReadRequiredValue("uid"),
                     UserName = game.ReadRequiredValue("username"),
                     Password = DPAPI.Decrypt(game.ReadRequiredValue("passwordhash"))
                 };
                 if (game["statisticsserviceenabled"]!=null)
                 {
                     gameSource.StatisticsServiceEnabled = game["statisticsserviceenabled"].Value<bool>();
                 }
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
							writer.WritePropertyName("statisticsserviceenabled");
                            writer.WriteValue(Settings.StatisticsServiceEnabled.Value);
                        }
                        writer.WriteRequiredValue("username", Settings.UserName ?? string.Empty);
                        writer.WriteRequiredValue("passwordhash",DPAPI.Encrypt(Settings.Password ?? string.Empty));

                        writer.WriteEndObject();

                        writer.WriteEndObject();
                    }
                }
            }
        }
    }
}