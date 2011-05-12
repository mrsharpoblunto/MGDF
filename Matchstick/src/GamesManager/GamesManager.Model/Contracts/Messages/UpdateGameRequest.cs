using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class UpdateGameRequest: RequestBase
    {
        [DataMember]
        public string GameUid { get; set; }

        [DataMember]
        public List<GameVersionUpdate> GameVersions { get; set; }
    }
}
