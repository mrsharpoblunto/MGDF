using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.Mapping;
using MGDF.GamesManager.GameSource.Model;

namespace MGDF.GamesManager.GameSource
{
    public class CommonService
    {
        public static void GetGamesHandler(GetGamesRequest request, GetGamesResponse response)
        {
            Developer matchingDeveloper = string.IsNullOrEmpty(request.DeveloperUid) ? null : GameSourceRepository.Current.Get<Developer>().SingleOrDefault(d => d.Uid == request.DeveloperUid);

            var predicate = PredicateBuilder.True<Game>();

            if (matchingDeveloper != null)
            {
                predicate = predicate.And(g => g.DeveloperId == matchingDeveloper.Id);
            }
            if (!string.IsNullOrEmpty(request.GameUid))
            {
                predicate = predicate.And(g => g.Uid == request.GameUid);
            }
            predicate = predicate.And(g => request.InterfaceVersion == g.InterfaceVersion);

            var games = GameSourceRepository.Current.Get<Game>().Where(predicate).OrderBy(g => g.Name);
            response.Games = GameMapper.MapToContractEntities(games);
        }
    }
}
