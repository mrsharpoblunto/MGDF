using System.Collections.Generic;
using System.Runtime.Serialization;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class GetInstalledGameInfoResponse : ResponseBase
    {
        [DataMember]
        public InstalledGameInfo Info { get; set; }
    }
}