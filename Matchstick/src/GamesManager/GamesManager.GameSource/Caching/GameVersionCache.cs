using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Mapping;
using MGDF.GamesManager.GameSource.Model;
using Game=MGDF.GamesManager.GameSource.Model.Game;
using GameVersion=MGDF.GamesManager.GameSource.Model.GameVersion;

namespace MGDF.GamesManager.GameSource.Caching
{
    class GameVersionCacheRequest
    {
        public Game Game;
        public Version Version;
    }

    class GameVersionCache : EntityCache<Contracts.Entities.GameVersionUpdate, GameVersionCacheRequest>
    {
        private static readonly GameVersionCache _instance = new GameVersionCache();
        public static GameVersionCache Instance { get { return _instance; } }

        protected override string GetCacheKey(GameVersionCacheRequest request)
        {
            return request.Game.Uid + request.Version;
        }

        protected override GameVersionUpdate CreateCacheEntry(GameVersionCacheRequest request)
        {
            var gameVersions = from gv in GameSourceRepository.Current.Get<GameVersion>() where gv.GameId == request.Game.Id && gv.IsUpdate && gv.Published select gv;

            GameVersion latestVersion = null;
            foreach (var gv in gameVersions)
            {
                if (request.Version >= new Version(gv.UpdateMinVersion) && request.Version <= new Version(gv.UpdateMaxVersion))
                {
                    if (latestVersion == null || new Version(gv.Version) > new Version(latestVersion.Version))
                    {
                        latestVersion = gv;
                    }
                }
            }

            return GameMapper.MapToContractEntity(request.Game, latestVersion);
        }
    }
}