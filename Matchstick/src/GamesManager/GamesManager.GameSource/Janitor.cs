using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource
{
    public class Janitor
    {
        private static readonly Janitor _instance = new Janitor();
        
        public static Janitor Instance
        {
            get { return _instance; }
        }

        private IServerContext _serverContext;
        private Thread _cleanupFragmentsThread;
        private Thread _processPendingDeletionsThread;

        private Janitor()
        {
        }

        public void Start()
        {
            if (_cleanupFragmentsThread == null)
            {
                _serverContext = new ServerContext(HttpContext.Current);
                _cleanupFragmentsThread = new Thread(DoCleanupExpiredFragments);
                _cleanupFragmentsThread.Start();

                _processPendingDeletionsThread = new Thread(DoProcessPendingDeletions);
                _processPendingDeletionsThread.Start();
            }
        }

        private static void DoCleanupExpiredFragments()
        {
            while (true)
            {
                try
                {
                    using (var repository = new GameSourceRepository(Config.Current.ConnectionString))
                    {
                        //clean up any old incomplete game versions/fragments
                        var expiryTime = TimeService.Current.Now.AddDays(-5);
                        var oldFragments = repository.Get<GameFragment>().Where(f => f.CreatedDate < expiryTime);
                        foreach (var fragment in oldFragments)
                        {
                            GameFragment frag = fragment;
                            GameVersion gameVersion = (from g in GameSourceRepository.Current.Get<GameVersion>() where g.Id == frag.GameVersionId select g).SingleOrDefault();

                            var pending = new PendingDelete
                            {
                                Id = Guid.NewGuid(),
                                GameDataId = fragment.GameDataId
                            };
                            repository.Insert(pending);
                            repository.Delete(fragment);
                            repository.Delete(gameVersion);
                        }
                        if (oldFragments.Count() > 0)
                        {
                            repository.SubmitChanges();
                        }
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex,"Error cleaning up expired incomplete fragments");
                }
                Thread.Sleep(1800000);//check once every 30 minutes
            }
        }

        private void DoProcessPendingDeletions()
        {
            while (true)
            {
                try
                {
                    using (var repository = new GameSourceRepository(Config.Current.ConnectionString))
                    {
                        //clean up any queued file deletions
                        var pendingDeletes = repository.Get<PendingDelete>();
                        bool submitChanges = false;
                        foreach (var pending in pendingDeletes) 
                        {
                            try
                            {
                                FileServer.Current.DeleteGameData(pending.GameDataId, _serverContext, repository);
                                repository.Delete(pending);
                                submitChanges = true;
                            }
                            catch (Exception ex)
                            {
                                Logger.Current.Write(ex, "Unable to delete game data ID=" + pending.GameDataId);
                            }
                        }
                        if (submitChanges) repository.SubmitChanges();
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex, "Error processing pending deletions");
                }
                Thread.Sleep(60000);//check once every minute
            }            
        }
    }
}
