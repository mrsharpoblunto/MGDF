
using System;
using System.IO;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using InstalledProgramsHelper=MGDF.GamesManager.Model.Helpers.InstalledProgramsHelper;

namespace MGDF.GamesManager.Model
{
    public class GameRegistrar: LongRunningTask
    {
        private bool _register;
        private Game _game;

        public GameRegistrar(bool register,Game game)
        {
            _register = register;
            _game = game;

        }

        public override LongRunningTaskResult Start()
        {
            LongRunningTaskResult result = LongRunningTaskResult.Completed;
            try
            {
                AddToInstalledPrograms();
                AddToGamesExplorer();
                AddToStartMenu();
                AddDesktopShortcut();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Install failed");
                result = LongRunningTaskResult.Error;
            }

            return result;
        }

        private void AddToGamesExplorer()
        {
            //add the game to the game explorer if we're on vista/win7 and the Game has a gdf file
            if (FileSystem.Current.FileExists(Resources.GameDefinitionFileBin()))
            {
                try
                {
                    if (GameExplorer.Current.IsInstalled(Resources.GameDefinitionFileBin()))
                    {
                        GameExplorer.Current.UninstallGame(Resources.GameDefinitionFileBin());
                    }

                    if (_register)
                    {
                        Guid instanceId = GameExplorer.Current.InstallGame(
                            Resources.GameDefinitionFileBin(),
                            EnvironmentSettings.Current.AppDirectory,
                            Resources.GamesManagerExecutable,
                            Resources.GamesManagerBootArguments(string.Empty, string.Empty, string.Empty, string.Empty));

                        //add homepage link
                        if (!string.IsNullOrEmpty(_game.Homepage))
                        {
                            GameExplorer.Current.AddGameTask(instanceId, TaskCount.One, "Homepage", _game.Homepage, string.Empty);
                        }
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex, "Unable to add game to games explorer");
                }
            }
        }

        private void AddDesktopShortcut()
        {
            string shortcutPath = Path.Combine(EnvironmentSettings.Current.CommonDesktopDirectory, _game.Name + ".lnk");
            if (ShortcutManager.Current.HasShortcut(shortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(shortcutPath);
            }

            if (_register)
            {
                ShortcutManager.Current.CreateShortcut(shortcutPath, Resources.GamesManagerExecutable, Resources.GamesManagerBootArguments(string.Empty, string.Empty, string.Empty, string.Empty), Resources.GameSystemIconFile());
            }
        }

        private void AddToStartMenu()
        {
            var startMenuFolder = FileSystem.Current.GetDirectory(Path.Combine(EnvironmentSettings.Current.CommonStartMenuDirectory, _game.DeveloperName));
            if (_register)
            {
                if (!startMenuFolder.Exists)
                {
                    startMenuFolder.Create();
                }
            }

            string shortcutPath = Path.Combine(startMenuFolder.FullName, _game.Name + ".lnk");
            if (ShortcutManager.Current.HasShortcut(shortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(shortcutPath);
            }

            if (_register)
            {
                ShortcutManager.Current.CreateShortcut(shortcutPath, Resources.GamesManagerExecutable, Resources.GamesManagerBootArguments(string.Empty, string.Empty, string.Empty, string.Empty), Resources.GameSystemIconFile());
            }
        }

        private void AddToInstalledPrograms()
        {
            InstalledProgramsHelper.RemoveFromInstalledPrograms(_game.Uid);
            if (_register)
            {
                InstalledProgramsHelper.AddToInstalledPrograms(_game);
            }
        }
    }
}