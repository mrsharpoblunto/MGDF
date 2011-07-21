using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.Model
{
    public class FrameworkUpdater: LongRunningTask
    {
        private readonly string _installer;

        public FrameworkUpdater(string installer)
        {
            _installer = installer;
        }

        public override LongRunningTaskResult Start()
        {
            LongRunningTaskResult result = LongRunningTaskResult.Completed;
            try
            {
                UpdateFramework();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Framework installation failed");
                result = LongRunningTaskResult.Error;
            }
            finally
            {
                Cleanup();
            }
            return result;
        }

        private void UpdateFramework()
        {
            IArchiveFile archive = ArchiveFactory.Current.OpenArchive(_installer);

            long total = ArchiveFileHelper.GetSubTreeData(archive).BytesCount;
            uint uTotal = total > uint.MaxValue ? uint.MaxValue : (uint)total;
            Total = uTotal;

            var vfsUtils = new ArchiveFileHelper();
            vfsUtils.OnCopyProgress += (sender, e) => Progress += (uint)e.BytesCopied;

            foreach (var child in archive.Children)
            {
                if (child.Value.IsFolder)
                {
                    vfsUtils.CopyVfsSubtree(child.Value, EnvironmentSettings.Current.AppDirectory, false);
                }
                //can't overwrite the currently executing file
                else if (child.Key.Equals("GamesManager.exe",StringComparison.InvariantCultureIgnoreCase))
                {
                    vfsUtils.CopyVfsFile(child.Value, FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, "GamesManager.New.exe"));
                }
                else
                {
                    vfsUtils.CopyVfsFile(child.Value, FileSystem.Combine(EnvironmentSettings.Current.AppDirectory, child.Value.Name));
                }
            }
        }

        private void Cleanup()
        {
            try
            {
                FileSystem.Current.GetFile(_installer).DeleteWithTimeout();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unable to delete framework installer file");
            }
        }
    }
}