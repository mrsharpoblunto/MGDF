using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using MGDF.GamesManager.GameSource.Model;

namespace MGDF.GamesManager.GameSource.Caching
{
    class GameCache: EntityCache<Game,string>
    {
        private static readonly GameCache _instance = new GameCache();
        public static GameCache Instance { get { return _instance; } }

        protected override string GetCacheKey(string request)
        {
            return request;
        }

        protected override Game CreateCacheEntry(string request)
        {
            return (from g in GameSourceRepository.Current.Get<Game>() where g.Uid == request select g).SingleOrDefault();
        }
    }
}
