using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Helpers;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.Model.Services;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;
using IFile=MGDF.GamesManager.Common.Framework.IFile;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    class PendingGameInstall: PendingOperationBase
    {
        private readonly IGameInstall _installer;
        private bool _removeInstallerAfterInstall;
        private string _sourceInstallerFileName;

        public PendingGameInstall(IGameInstall installer, bool removeInstallerAfterInstall, ReaderWriterLockSlim syncLock)
            : base(syncLock)
        {
            _removeInstallerAfterInstall = removeInstallerAfterInstall;
            _sourceInstallerFileName = installer.InstallerFile;

            _installer = installer;
            Name = _installer.IsUpdate ? "Installing Update" : "Installing";
        }

        public PendingGameInstall(XmlReader reader, ReaderWriterLockSlim syncLock)
            : base(reader,syncLock)
        {
            _installer = EntityFactory.Current.CreateGameInstall(_sourceInstallerFileName);
            Name = _installer.IsUpdate ? "Installing Update" : "Installing";

        }

        private void Cleanup()
        {
            _installer.GameContents.Dispose();
            try
            {
                if (_removeInstallerAfterInstall)
                {
                    FileSystem.Current.GetFile(_installer.InstallerFile).DeleteWithTimeout();
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unable to delete pending install file");
            }
        }

        public override IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock, out bool reloadGame)
        {
            reloadGame = true;
            return null;
        }

        protected override void Work()
        {
            if (ValidateInstaller(_installer))
            {
                if (_installer.IsUpdate)
                {
                    UpdateGame(_installer);
                }
                else
                {
                    InstallGame(_installer);
                }

                if (!PausePending && !ErrorPending)
                {
                    AddToGamesExplorer();
                    AddToInstalledPrograms();
                    AddToStartMenu();
                    AddDesktopShortcut();
                }
            }
            else
            {
                Abort("Installer is invalid");
            }
        }

        private bool ValidateInstaller(IGameInstall install)
        {
            if (!install.IsValid)
            {
                foreach (var error in _installer.ErrorCollection)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to install Game - " + error);
                }
                return false;
            }
            return true;
        }

        private void AddToGamesExplorer()
        {
            //add the game to the game explorer if we're on vista/win7 and the Game has a gdf file
            if (FileSystem.Current.FileExists(Constants.GameDefinitionFileBin(_installer.Game.Uid)))
            {
                try
                {
                    if (GameExplorer.Current.IsInstalled(Constants.GameDefinitionFileBin(_installer.Game.Uid)))
                    {
                        GameExplorer.Current.UninstallGame(Constants.GameDefinitionFileBin(_installer.Game.Uid));
                    }

                    Guid instanceId = GameExplorer.Current.InstallGame(
                        Constants.GameDefinitionFileBin(_installer.Game.Uid),
                        EnvironmentSettings.Current.AppDirectory,
                        Constants.BootStrapperExecutable,
                        Constants.GamesManagerBootArguments(_installer.Game.Uid));

                    //add homepage link
                    if (!string.IsNullOrEmpty(_installer.Game.Homepage))
                    {
                        GameExplorer.Current.AddGameTask(instanceId, TaskCount.One, "Homepage",_installer.Game.Homepage, string.Empty);
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex,"Unable to add game to games explorer");
                }
            }
        }

        private void AddDesktopShortcut()
        {
            string shortcutPath = Path.Combine(EnvironmentSettings.Current.CommonDesktopDirectory, _installer.Game.Name + ".lnk");
            if (ShortcutManager.Current.HasShortcut(shortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(shortcutPath);
            }
            ShortcutManager.Current.CreateShortcut(shortcutPath, Constants.BootStrapperExecutable, Constants.GamesManagerBootArguments(_installer.Game.Uid), Constants.GameSystemIconFile(_installer.Game.Uid));
        }

        private void AddToStartMenu()
        {
            var startMenuFolder = FileSystem.Current.GetDirectory(Path.Combine(EnvironmentSettings.Current.CommonStartMenuDirectory,_installer.Game.DeveloperName));
            if (!startMenuFolder.Exists)
            {
                startMenuFolder.Create();
            }

            string shortcutPath = Path.Combine(startMenuFolder.FullName, _installer.Game.Name + ".lnk");
            if (ShortcutManager.Current.HasShortcut(shortcutPath))
            {
                ShortcutManager.Current.DeleteShortcut(shortcutPath);
            }
            ShortcutManager.Current.CreateShortcut(shortcutPath, Constants.BootStrapperExecutable, Constants.GamesManagerBootArguments(_installer.Game.Uid), Constants.GameSystemIconFile(_installer.Game.Uid));
        }

        private void AddToInstalledPrograms()
        {
            InstalledProgramsHelper.RemoveFromInstalledPrograms(_installer.Game.Uid);
            InstalledProgramsHelper.AddToInstalledPrograms(_installer.Game);
        }

        private void InstallGame(IGameInstall installer)
        {
            IDirectory gameDir = FileSystem.Current.GetDirectory(Constants.GameDir(installer.Game.Uid));
            if (!gameDir.Exists)
            {
                gameDir.Create();
            }

            IArchiveFile gameConfig = installer.GameContents.GetFile(Constants.GameConfig);
            IArchiveFile contentDir = installer.GameContents.GetFile(Constants.ContentDir);
            IArchiveFile binDir = installer.GameContents.GetFile(Constants.BinDir);
            IArchiveFile imageFile = null;
            if (installer.Game.GameIconData!=null)
            {
                imageFile = installer.GameContents.GetFile(Constants.GameIcon);
            }
            IArchiveFile gdfFile = installer.GameContents.GetFile(Constants.GameDefinitionFileBinary);
            IArchiveFile preferencesFile = installer.GameContents.GetFile(Constants.PreferencesConfig);

            var vfsUtils = new ArchiveFileHelper();
            
            var total = ArchiveFileHelper.GetSubTreeData(gameConfig).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(contentDir).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(binDir).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(imageFile).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(gdfFile).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(preferencesFile).BytesCount;

            UpdateTotal(total);

            vfsUtils.OnCopyProgress += (sender, e) =>
                                           {
                                               UpdateProgress(Progress + e.BytesCopied);
                                               if (PausePending)
                                               {
                                                   e.Cancel = true;
                                               }
                                           };

            vfsUtils.CopyVfsFile(gameConfig, FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.GameConfig));

            if (!PausePending && contentDir != null)
            {
                vfsUtils.CopyVfsSubtree(contentDir, Constants.GameDir(installer.Game.Uid), false);
            }

            if (!PausePending && binDir != null)
            {
                vfsUtils.CopyVfsSubtree(binDir, Constants.GameDir(installer.Game.Uid), false);
            }

            if (!PausePending && imageFile != null)
            {
                vfsUtils.CopyVfsFile(imageFile, FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.GameIcon));
                IconManager.Current.CreateIcon(installer.Game.Name, FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.GameIcon), FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.GameSystemIcon));
            }

            if (!PausePending && gdfFile!=null)
            {
                vfsUtils.CopyVfsFile(gdfFile, FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.GameDefinitionFileBinary));
            }

            if (!PausePending && preferencesFile!=null)
            {
                vfsUtils.CopyVfsFile(preferencesFile, FileSystem.Combine(Constants.GameDir(installer.Game.Uid), Constants.PreferencesConfig));
            }
        }

        private void UpdateGame(IGameInstall installer)
        {
            IDirectory gameDir = FileSystem.Current.GetDirectory(Constants.GameDir(_installer.Game.Uid));
            IDirectory gameContentDir = FileSystem.Current.GetDirectory(FileSystem.Combine(Constants.GameDir(_installer.Game.Uid), Constants.ContentDir));
            IDirectory gameModulesDir = FileSystem.Current.GetDirectory(FileSystem.Combine(Constants.GameDir(_installer.Game.Uid), Constants.BinDir));

            //only allow files/dirs inside the modules/content directories to be removed by the removal process.
            foreach (string s in installer.Update.RemoveFiles)
            {
                IFile file = FileSystem.Current.GetFile(FileSystem.Combine(gameDir.FullName, s));
                if (file.Exists && (ExistsInsideGameDirectory(file, gameContentDir) || ExistsInsideGameDirectory(file, gameModulesDir)))
                {
                    file.DeleteWithTimeout();
                    continue;
                }

                IDirectory dir = FileSystem.Current.GetDirectory(FileSystem.Combine(gameDir.FullName, s));
                if (dir.Exists && (ExistsInsideGameDirectory(dir, gameContentDir) || ExistsInsideGameDirectory(dir, gameModulesDir)))
                {
                    dir.DeleteWithTimeout();
                }
            }

            InstallGame(installer);
        }

        private static bool ExistsInsideGameDirectory(IDirectory dir, IDirectory parent)
        {
            if (dir.FullName == parent.FullName) return false; //can't delete the parent directory, only children of it

            while (dir != null)
            {
                if (dir.FullName == parent.FullName) return true;
                dir = dir.Parent;
            }
            return false;
        }

        private static bool ExistsInsideGameDirectory(IFile file, IDirectory parent)
        {
            IDirectory dir = file.Parent;
            while (dir != null)
            {
                if (dir.FullName == parent.FullName) return true;
                dir = dir.Parent;
            }
            return false;
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (!LoadBase(reader))
                        {
                            switch (reader.Name)
                            {
                                case "sourceinstallerfilename":
                                    _sourceInstallerFileName = reader.ReadString();
                                    break;
                                case "removeinstallerafterinstall":
                                    _removeInstallerAfterInstall = bool.Parse(reader.ReadString());
                                    break;
                            }
                        }
                        break;
                }
            }
        }

        /// <summary>
        /// persist the fact that we are installing so if the process is interrupted part way through we can resume the install later
        /// </summary>
        public override void Save(XmlWriter writer)
        {
            writer.WriteStartElement("pendinggameinstall");
            SaveBase(writer);
            writer.WriteElementString("sourceinstallerfilename", _sourceInstallerFileName);
            writer.WriteStartElement("removeinstallerafterinstall", _removeInstallerAfterInstall.ToString());
            writer.WriteEndElement();
        }

        protected override void Paused()
        {
        }

        protected override void Finished()
        {
            Cleanup();
        }

        protected override void Error()
        {
            FileSystem.Current.GetFile(Path.Combine(Constants.GameDir(_installer.Game.Uid),Constants.ErrorFile)).WriteText("Install Failed");
            Cleanup();
        }
    }
}