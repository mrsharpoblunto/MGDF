using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGamesResponse: ResponseBase
    {
        private List<Game> _games = new List<Game>();

        [DataMember]
        public List<Game> Games
        {
            get { return _games; }
            set { _games = value; }
        }
    }
}
