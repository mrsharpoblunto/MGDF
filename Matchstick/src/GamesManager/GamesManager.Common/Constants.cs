using System;
using System.Collections.Generic;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common
{
    public static class Constants
    {
        public const int AuthenticationTimeoutMinutes = 15;

        public static class Headers
        {
            public const string FragmentIdHeader = "X-MGDF-FragmentId";
            public const string IsLastFragment = "X-MGDF-IsLastFragment";
            public const string DeveloperKey = "X-MGDF-DeveloperKey";
            public const string Hmac = "X-MGDF-HMAC";
            public const string Timestamp = "X-MGDF-Timestamp";
            public const string Md5 = "X-MGDF-MD5";
            public const string CNonce = "X-MGDF-CNonce";
            public const string MzaContentType = "application/x-mgdf-archive";
        }

        public static class QueryString
        {
            public const string Hmac = "hmac";
            public const string Timestamp = "timestamp";            
        }

        public const int InterfaceVersion = 1;
        public const string SupportEmail = "support@junkship.org";

        public const string ErrorFile = ".error";
        public const string GameStateConfig = @"gamestate.xml";
        public const string UpdateConfig = @"update.xml";
        public const string GameConfig = @"game.xml";
        public const string PreferencesConfig = @"preferences.xml";
        public const string PendingOperationQueue = @"pendingoperationqueue.xml";
        public const string Statistics = @"statistics.txt";
        public const string GameSplashImage = "splashimage.png";
        public const string GameIcon = "gameicon.png";
        public const string GameSystemIcon = "gamesystemicon.ico";
        public const string GameDefinitionFileBinary = "gdf.dll";
        public const int DefaultPort = 2213;

        public const string CoreGame = "core";
        public const string ContentDir = @"content";
        public const string BinDir = @"bin";
        public const string GamesDir = @"games";
        public const string SchemaDir = @"schemas";

        public const string MGDFZipArchiveExtension = ".mza";

        public static string MGDFExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory,"core.exe");
            }
        }

        public static string BootStrapperExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.BootStrapper.exe");
            }
        }

        public static string FrameworkUpdaterExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.FrameworkUpdater.exe");
            }
        }

        public static string CurrentFrameworkUpdaterExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.FrameworkUpdater.Current.exe");
            }
        }

        public static string FrameworkUpdaterConfig
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.FrameworkUpdater.exe.config");
            }
        }

        public static string CurrentFrameworkUpdaterConfig
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.FrameworkUpdater.Current.exe.config");
            }
        }

        public static string GamesManagerExecutable
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory,"GamesManager.exe");
            }
        }

        public static string MGDFBootArguments(string gameUid)
        {
            return "-boot:" + gameUid + 
                (!string.IsNullOrEmpty(GamesDirOverride) ? (" -gamesdiroverride:\""+GamesDirOverride+"\""):string.Empty)+
                " -hideerrors";
        }

        public static class GamesManagerArguments
        {
            public const string InstallArgument = "install";
            public const string BootArgument = "boot";
            public const string UninstallArgument = "uninstall";
            public const string GamesDirOverrideArgument = "gamesdiroverride";
            public const string NoUpdateCheckArgument = "noupdatecheck";
            public const string CheckUpdatesArgument = "checkupdates";
            public const string CheckUninstallArgument = "checkuninstall";
        }

        public static string GamesManagerBootArguments(string uid)
        {
            return "-" + GamesManagerArguments.BootArgument + ":"+uid;
        }

        public static string GamesManagerUninstallArguments(string uid)
        {
            return "-" + GamesManagerArguments.UninstallArgument + ":" + uid;
        }

        public static string GamesManagerCheckUpdatesArguments(string uid)
        {
            return "-" + GamesManagerArguments.CheckUpdatesArgument + ":" + uid;
        }

        public static string CorePreferences
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory,GamesDir,CoreGame,PreferencesConfig);
            }
        }

        public static string FrameworkUpdateFile
        {
            get
            {
                return FileSystem.Combine(FrameworkUpdatesDir, "pendingframeworkdownload.xml");
            }
        }

        public static string GamesDirOverride { get; set; }

        public static string GameDir(string gameUid)
        {
            return FileSystem.Combine(GamesBaseDir, gameUid);
        }

        public static string GameDefinitionFileBin(string gameUid)
        {
            return FileSystem.Combine(GameDir(gameUid), GameDefinitionFileBinary);
        }

        public static string GameIconFile(string gameUid)
        {
            if (FileSystem.Current.FileExists(FileSystem.Combine(GameDir(gameUid), GameIcon)))
            {
                return FileSystem.Combine(GameDir(gameUid), GameIcon);
            }
            else
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, GamesDir, CoreGame, GameIcon);
            }
        }

        public static string GameSystemIconFile(string gameUid)
        {
            if (FileSystem.Current.FileExists(FileSystem.Combine(GameDir(gameUid), GameSystemIcon)))
            {
                return FileSystem.Combine(GameDir(gameUid), GameSystemIcon);
            }
            else
            {
                return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, GamesDir, CoreGame, GameSystemIcon);
            }
        }

        public static string PendingOperationQueueFile(string gameUid)
        {
            return FileSystem.Combine(GameDir(gameUid), PendingOperationQueue);
        }

        public static string GamesBaseDir
        {
            get
            {
                return GamesDirOverride ?? FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, GamesDir);
            }
        }

        public static string DownloadsDir
        {
            get
            {
                return FileSystem.Combine(GamesBaseDir, "Downloads");
            }
        }

        public static string FrameworkUpdatesDir
        {
            get
            {
                return FileSystem.Combine(DownloadsDir, "Framework");
            }
        }

        public static void CreateRequiredCommonDirectories()
        {
            var gamesBaseDir = FileSystem.Current.GetDirectory(GamesBaseDir);
            if (!gamesBaseDir.Exists)
            {
                gamesBaseDir.Create();
            }
            var downloadsDir = FileSystem.Current.GetDirectory(DownloadsDir);
            if (!downloadsDir.Exists)
            {
                downloadsDir.Create();
            }
            var frameworkUpdatesDir = FileSystem.Current.GetDirectory(FrameworkUpdatesDir);
            if (!frameworkUpdatesDir.Exists)
            {
                frameworkUpdatesDir.Create();
            }
        }

        #region user specific constants. Dont use in windows service

        public static string GamesUserBaseDir
        {
            get
            {
                return FileSystem.Combine(EnvironmentSettings.Current.UserDirectory, GamesDir);
            }
        }

        public static string UserGameLastUpdateFile(string gameUid)
        {
            return FileSystem.Combine(GameUserDir(gameUid), ".lastupdate");
        }

        public static string UserGameLockFile(string gameUid)
        {
            return FileSystem.Combine(GameUserDir(gameUid), ".lock");
        }

        public static string GameUserDir(string gameUid)
        {
            var gameUserDir = FileSystem.Current.GetDirectory(FileSystem.Combine(GamesUserBaseDir, gameUid));
            if (!gameUserDir.Exists)
            {
                gameUserDir.Create();
            }
            return gameUserDir.FullName;
        }

        public static string UserStatistics(string gameUid)
        {
            return FileSystem.Combine(GameUserDir(gameUid), Statistics);
        }

        public static void CreateRequiredUserDirectories()
        {
            var userGamesBaseDir = FileSystem.Current.GetDirectory(GamesUserBaseDir);
            if (!userGamesBaseDir.Exists)
            {
                userGamesBaseDir.Create();
            }
        }

        #endregion
    }
}
