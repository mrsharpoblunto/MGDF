using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class User
    {
        [DataMember]
        public Guid Id { get; set; }

        [DataMember]
        public string CustomUserData { get; set; }
    }
}
