using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Messages;
using MGDF.GamesManager.Model.Entities.XmlEntities;

namespace MGDF.GamesManager.Model.ServiceModel
{
    class InstalledGamesList
    {
        private readonly Dictionary<string,InstalledGame> _cache = new Dictionary<string, InstalledGame>();

        public InstalledGame Get(string uid)
        {
            if (!_cache.ContainsKey(uid))
            {
                lock (_cache)
                {
                    if (!_cache.ContainsKey(uid))
                    {
                        InstalledGame item;
                        if (FileSystem.Current.DirectoryExists(Constants.GameDir(uid)))
                        {
                            item = new InstalledGame(FileSystem.Current.GetDirectory(Constants.GameDir(uid)));
                        }
                        else
                        {
                            item = new InstalledGame();
                        }
                        _cache.Add(uid, item);
                    }
                }
            }
            return _cache[uid];
        }

        public void PauseAll()
        {
            lock (_cache)
            {
                foreach (var kvp in _cache)
                {
                    kvp.Value.Pause();
                }
            }
        }

        public void WaitUntilAllPaused()
        {
            var items = new List<InstalledGame>();
            lock (_cache)
            {
                foreach (var kvp in _cache)
                {
                    items.Add(kvp.Value);
                }
            }

            bool allPaused;
            do
            {
                allPaused = true;
                foreach (var item in items)
                {
                    var itemInfo = item.GetInfo();
                    if (itemInfo.InstallState == InstallState.Updating &&
                        itemInfo.PendingOperations.Count > 0 &&
                        itemInfo.PendingOperations[0].Status != PendingOperationStatus.Paused)
                    {
                        allPaused = false;
                        Thread.Sleep(100);
                        break;
                    }
                }
            } while (!allPaused);
        }
    }
}
