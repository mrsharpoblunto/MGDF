﻿using System;
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
      public const string GameDirOverrideArgument = "gamediroverride";
    }

    public const int InterfaceVersion = 1;
    public const string SupportUrl = "https://github.com/mrsharpoblunto/MGDF";

    public const string DependencyConfig = @"dependencies.json";
    public const string UpdateConfig = @"update.json";
    public const string GameConfig = @"game.json";
    public const string PreferencesConfig = @"preferences.json";
    public const string GameIcon = "gameicon.png";
    public const string GameSystemIcon = "gamesystemicon.ico";
    public const string LastUpdate = ".lastupdate";

    public const string CoreResources = "resources";
    public const string ContentDir = @"content";
    public const string BinDir = @"bin";
    public const string GameDir = @"game";
    public const string SchemaDir = @"schemas";

    private static string _gameUserDir;
    private static string _gameDir;
    private static bool _userDirOverridden;

    public static string MGDFExecutable
    {
      get
      {
        return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "core.exe");
      }
    }

    public static string ParamsOverrideFile
    {
      get
      {
        return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "params.txt");
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
        return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.exe");
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
      return "-hideerrors" + (!string.IsNullOrEmpty(_gameDir) ? (" -gamediroverride \"" + _gameDir + "\"") : string.Empty) + (_userDirOverridden ? " -userdiroverride" : string.Empty);
    }

    public static string StatisticsServiceArguments()
    {
      return " -statisticsenabled";
    }

    public static string LauncherArguments(long windowHandle)
    {
      return " -launcherhandle " + windowHandle;
    }

    public static string GamesManagerBootArguments(string gameUpdate, string gameUpdateHash, string frameworkUpdate, string frameworkUpdateHash)
    {
      return (!string.IsNullOrEmpty(gameUpdate) ? (" -updategame \"" + gameUpdate + "\" -gameupdatehash \"" + gameUpdateHash + "\"") : string.Empty) +
             (!string.IsNullOrEmpty(frameworkUpdate) ? (" -updateframework \"" + frameworkUpdate + "\" -frameworkupdatehash \"" + frameworkUpdateHash + "\"") : string.Empty) +
             (_userDirOverridden ? "-userdiroverride" : string.Empty);
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
        return !string.IsNullOrEmpty(_gameDir) ? _gameDir : FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, GameDir);
      }
    }

    public static string DownloadsDir
    {
      get
      {
        return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "Downloads");
      }
    }

    public static string DependenciesDir
    {
      get
      {
        return FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "Dependencies");
      }
    }

    public static string DependenciesFile
    {
      get
      {
        return FileSystem.Combine(DependenciesDir, DependencyConfig);
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

    public static void UninitUserDirectory()
    {
      _userDirOverridden = false;
      _gameUserDir = null;
    }

    public static void InitGameDirectory(string gameDirOverride)
    {
      _gameDir = gameDirOverride;
    }

    public static void InitUserDirectory(string gameUid, bool useApplicationRoot)
    {
      if (!useApplicationRoot)
      {
        _gameUserDir = Path.Combine(EnvironmentSettings.Current.UserDirectory, gameUid);
      }
      else if (!string.IsNullOrEmpty(_gameDir))
      {
        _gameUserDir = Path.Combine(Path.GetDirectoryName(_gameDir), "user");
      }
      else
      {
        _gameUserDir = Path.Combine(EnvironmentSettings.Current.AppDirectory, "user");
        var userGamesBaseDir = FileSystem.Current.GetDirectory(_gameUserDir);
        if (!userGamesBaseDir.Exists)
        {
          userGamesBaseDir.Create();
        }
        _gameUserDir = Path.Combine(_gameUserDir, gameUid);
      }
      _userDirOverridden = useApplicationRoot;

      var userGamesDir = FileSystem.Current.GetDirectory(_gameUserDir);
      if (!userGamesDir.Exists)
      {
        userGamesDir.Create();
      }
    }
  }
}
