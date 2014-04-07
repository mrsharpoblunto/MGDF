using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common
{
    public static class Resources
    {
        public static class GamesManagerArguments
        {
            public const string RegisterArgument = "register";
            public const string DeregisterArgument = "deregister";
            public const string NoUpdateCheckArgument = "noupdatecheck";
            public const string UpdateFrameworkArgument = "updateframework";
            public const string UpdateGameArgument = "updategame";
            public const string GameUpdateHashArgument = "gameupdatehash";
            public const string FrameworkUpdateHashArgument = "frameworkupdatehash";
            public const string UserDirOverrideArgument = "userdiroverride";
        }

        public const int InterfaceVersion = 1;
        public const string SupportEmail = "support@matchstickframework.org";

		public const string DependancyConfig = @"dependancies.json";
        public const string UpdateConfig = @"update.json";
        public const string GameConfig = @"game.json";
        public const string PreferencesConfig = @"preferences.json";
        public const string Statistics = @"statistics.txt";
        public const string GameIcon = "gameicon.png";
        public const string GameSystemIcon = "gamesystemicon.ico";
        public const string GameDefinitionFileBinary = "gdf.dll";
		public const string LastUpdate = ".lastupdate";

        public const string CoreResources = "resources";
        public const string ContentDir = @"content";
        public const string BinDir = @"bin";
        public const string GameDir = @"game";
        public const string SchemaDir = @"schemas";

        private static string _gameUserDir;
        private static bool _userDirOverridden;

        public static string MGDFExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory,"core.exe");
            }
        }

        public static string FrameworkUpdaterExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.FrameworkUpdater.exe");
            }
        }

        public static string GamesManagerExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory,"GamesManager.exe");
            }
        }

        public static string GamesManagerNewExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.New.exe");
            }
        }

        public static string CoreBootArguments()
        {
            return "-hideerrors";
        }

        public static string GamesManagerBootArguments(string gameUpdate,string gameUpdateHash,string frameworkUpdate,string frameworkUpdateHash)
        {
            return (!string.IsNullOrEmpty(gameUpdate) ? (" -updategame:\"" + gameUpdate + "\" -gameupdatehash:\"" + gameUpdateHash + "\"") : string.Empty) +
                   (!string.IsNullOrEmpty(frameworkUpdate) ? (" -updateframework:\"" + frameworkUpdate + "\" -frameworkupdatehash:\"" + frameworkUpdateHash + "\"") : string.Empty) +
                   (_userDirOverridden ? "-userdiroverride" : string.Empty);
        }

        public static string GameDefinitionFileBin()
        {
            return FileSystem.Combine(GameBaseDir, GameDefinitionFileBinary);
        }

        public static string GameIconFile()
        {
            if (FileSystem.Current.FileExists(FileSystem.Combine(GameBaseDir, GameIcon)))
            {
                return FileSystem.Combine(GameBaseDir, GameIcon);
            }
            else
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, CoreResources, GameIcon);
            }
        }

        public static string GameSystemIconFile()
        {
            if (FileSystem.Current.FileExists(FileSystem.Combine(GameBaseDir, GameSystemIcon)))
            {
                return FileSystem.Combine(GameBaseDir, GameSystemIcon);
            }
            else
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, CoreResources, GameSystemIcon);
            }
        }

        public static string GameBaseDir
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, GameDir);
            }
        }

        public static string DownloadsDir
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "Downloads");
            }
        }

		public static string DependanciesDir
		{
			get
			{
				return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "Dependancies");
			}
		}

		public static string DependanciesFile
		{
			get
			{
				return FileSystem.Combine(DependanciesDir, DependancyConfig);
			}
		}

        public static void InitUpdaterDirectories()
        {
            var downloadsDir = FileSystem.Current.GetDirectory(DownloadsDir);
            if (!downloadsDir.Exists)
            {
                downloadsDir.Create();
            }
        }

        public static string GameUserDir
        {
            get
            {
                return _gameUserDir;
            }
        }

        public static string UserGameLastUpdateFile()
        {
            return FileSystem.Combine(GameUserDir, LastUpdate);
        }

        public static string UserStatistics()
        {
            return FileSystem.Combine(GameUserDir, Statistics);
        }

        public static void UninitUserDirectory()
        {
            _userDirOverridden = false;
            _gameUserDir = null;
        }

        public static void InitUserDirectory(string gameUid,bool useApplicationRoot)
        {
            if (useApplicationRoot) _gameUserDir = EnvironmentSettings.Current.AppDirectory;
            _userDirOverridden = useApplicationRoot;

            _gameUserDir = !string.IsNullOrEmpty(_gameUserDir) ? (_gameUserDir + "\\user") : EnvironmentSettings.Current.UserDirectory;
            var userGamesBaseDir = FileSystem.Current.GetDirectory(_gameUserDir);
            if (!userGamesBaseDir.Exists)
            {
                userGamesBaseDir.Create();
            }

            _gameUserDir = Path.Combine(_gameUserDir, gameUid);
            var userGamesDir = FileSystem.Current.GetDirectory(_gameUserDir);
            if (!userGamesDir.Exists)
            {
                userGamesDir.Create();
            }
        }
    }
}
