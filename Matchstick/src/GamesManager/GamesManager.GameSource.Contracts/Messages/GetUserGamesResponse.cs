using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetUserGamesResponse: ResponseBase
    {
        private List<UserGame> _userGames = new List<UserGame>();

        [DataMember]
        public List<UserGame> UserGames
        {
            get { return _userGames;  }
            set { _userGames = value; }
        }
    }
}