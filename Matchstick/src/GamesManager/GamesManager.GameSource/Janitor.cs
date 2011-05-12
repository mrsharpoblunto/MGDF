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
        private Thread _cleanupThread;

        private Janitor()
        {
        }

        public void Start()
        {
            if (_cleanupThread == null)
            {
                _serverContext = new ServerContext(HttpContext.Current);
                _cleanupThread = new Thread(DoCleanup);
                _cleanupThread.Start();
            }
        }

        private void DoCleanup()
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
                            GameFragment fragment1 = fragment;
                            GameVersion gameVersion = (from g in GameSourceRepository.Current.Get<GameVersion>() where g.Id == fragment1.GameVersionId select g).SingleOrDefault();

                            FileServer.Current.DeleteGameFragmentData(fragment, _serverContext,repository);
                            repository.Delete(fragment);
                            repository.Delete(gameVersion);
                        }

                        //clean up any queued file deletions
                        var results = repository.Get<QueuedFileDelete>();
                        var remove = new List<QueuedFileDelete>();
                        foreach (var result in results)
                        {
                            try
                            {
                                var file = FileSystem.Current.GetFile(result.FileName);
                                if (file.Exists) file.Delete();
                                remove.Add(result);
                            }
                            catch (Exception)
                            {
                            }
                        }
                        if (remove.Count > 0) repository.DeleteAll(remove);

                        if (oldFragments.Count() > 0 || remove.Count > 0)
                        {
                            repository.SubmitChanges();
                        }
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex,"Error cleaning up");
                }
                Thread.Sleep(300000);//check once every 5 minutes
            }
        }
    }
}
