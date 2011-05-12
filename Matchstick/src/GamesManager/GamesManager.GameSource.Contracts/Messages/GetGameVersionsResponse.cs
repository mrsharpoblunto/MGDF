using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGameVersionsResponse: ResponseBase
    {
        private List<GameVersion> _gameVersions = new List<GameVersion>();

        [DataMember]
        public List<GameVersion> GameVersions
        {
            get { return _gameVersions; }
            set { _gameVersions = value; }
        }
    }
}
