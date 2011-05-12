using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class GameVersionBase
    {
        [DataMember]
        public Guid Id { get; set; }

        [DataMember]
        public bool IsUpdate { get; set; }

        [DataMember]
        public string Description { get; set; }

        [DataMember]
        public string Version { get; set; }

        [DataMember]
        public string UpdateMaxVersion { get; set; }

        [DataMember]
        public string UpdateMinVersion { get; set; }

        [DataMember]
        public bool Published { get; set; }

        [DataMember]
        public string Md5Hash { get; set; }
    }
}
