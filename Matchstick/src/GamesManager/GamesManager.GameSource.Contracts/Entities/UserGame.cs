using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class UserGame: GameInfoBase
    {
        [DataMember]
        public UserGameVersion LatestVersion { get; set; }
    }
}
