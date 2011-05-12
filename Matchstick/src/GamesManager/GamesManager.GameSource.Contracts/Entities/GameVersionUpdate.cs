using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class GameVersionUpdate: GameVersionBase
    {
        [DataMember]
        public string DownloadURL { get; set; }

        [DataMember]
        public GameUpdate Game { get; set; }
    }
}
