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
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Model.Helpers;
using IFile=MGDF.GamesManager.Common.Framework.IFile;

namespace MGDF.GamesManager.Model
{
    public class GameUpdater: LongRunningTask
    {
        private readonly GameInstall _installer;

        public GameUpdater(GameInstall installer)
        {
            _installer = installer;
        }

        public override  LongRunningTaskResult Start()
        {
            LongRunningTaskResult result = LongRunningTaskResult.Completed;
            try 
            {
                if (ValidateInstaller(_installer))
                {
                    if (_installer.IsUpdate)
                    {
                        UpdatePartialGame(_installer);
                    }
                    else
                    {
                        UpdateFullGame(_installer);
                    }
                }
                else
                {
                    result = LongRunningTaskResult.Error;
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Install failed");
                result = LongRunningTaskResult.Error;
            }
            finally
            {
                Cleanup();
            }

            return result;
        }

        private bool ValidateInstaller(GameInstall install)
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

        private void UpdateFullGame(GameInstall installer)
        {
            IDirectory gameDir = FileSystem.Current.GetDirectory(Resources.GameBaseDir);
            if (!gameDir.Exists)
            {
                gameDir.Create();
            }

            IArchiveFile gameConfig = installer.GameContents.GetFile(Resources.GameConfig);
            IArchiveFile contentDir = installer.GameContents.GetFile(Resources.ContentDir);
            IArchiveFile binDir = installer.GameContents.GetFile(Resources.BinDir);
            IArchiveFile imageFile = null;
            if (installer.Game.GameIconData!=null)
            {
                imageFile = installer.GameContents.GetFile(Resources.GameIcon);
            }
            IArchiveFile gdfFile = installer.GameContents.GetFile(Resources.GameDefinitionFileBinary);
            IArchiveFile preferencesFile = installer.GameContents.GetFile(Resources.PreferencesConfig);

            var vfsUtils = new ArchiveFileHelper();
            
            var total = ArchiveFileHelper.GetSubTreeData(gameConfig).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(contentDir).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(binDir).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(imageFile).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(gdfFile).BytesCount;
            total += ArchiveFileHelper.GetSubTreeData(preferencesFile).BytesCount;

            uint uTotal = total > uint.MaxValue ? uint.MaxValue : (uint)total;
            Total = uTotal;

            vfsUtils.OnCopyProgress += (sender, e) => Progress += (uint) e.BytesCopied;

            vfsUtils.CopyVfsFile(gameConfig, FileSystem.Combine(Resources.GameBaseDir, Resources.GameConfig));

            if (contentDir != null)
            {
                vfsUtils.CopyVfsSubtree(contentDir, Resources.GameBaseDir, false);
            }

            if (binDir != null)
            {
                vfsUtils.CopyVfsSubtree(binDir, Resources.GameBaseDir, false);
            }

            if (imageFile != null)
            {
                vfsUtils.CopyVfsFile(imageFile, FileSystem.Combine(Resources.GameBaseDir, Resources.GameIcon));
                IconManager.Current.CreateIcon(installer.Game.Name, FileSystem.Combine(Resources.GameBaseDir, Resources.GameIcon), FileSystem.Combine(Resources.GameBaseDir, Resources.GameSystemIcon));
            }

            if (gdfFile!=null)
            {
                vfsUtils.CopyVfsFile(gdfFile, FileSystem.Combine(Resources.GameBaseDir, Resources.GameDefinitionFileBinary));
            }

            if (preferencesFile!=null)
            {
                vfsUtils.CopyVfsFile(preferencesFile, FileSystem.Combine(Resources.GameBaseDir, Resources.PreferencesConfig));
            }
        }

        private void UpdatePartialGame(GameInstall installer)
        {
            IDirectory gameDir = FileSystem.Current.GetDirectory(Resources.GameBaseDir);
            IDirectory gameContentDir = FileSystem.Current.GetDirectory(FileSystem.Combine(Resources.GameBaseDir, Resources.ContentDir));
            IDirectory gameModulesDir = FileSystem.Current.GetDirectory(FileSystem.Combine(Resources.GameBaseDir, Resources.BinDir));

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

            UpdateFullGame(installer);
        }

        private void Cleanup()
        {
            _installer.GameContents.Dispose();
            try
            {
                FileSystem.Current.GetFile(_installer.InstallerFile).DeleteWithTimeout();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unable to delete game installer file");
            }
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
    }
}