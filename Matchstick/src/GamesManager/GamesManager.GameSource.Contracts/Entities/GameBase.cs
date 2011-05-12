using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class GameBase
    {
        [DataMember]
        public string Uid { get; set; }

        [DataMember]
        public string Name { get; set; }

        [DataMember]
        public int InterfaceVersion { get; set; }

        [DataMember]
        public string Description { get; set; }

        [DataMember]
        public string Homepage { get; set; }

        [DataMember]
        public bool RequiresAuthentication { get; set; }
    }
}
