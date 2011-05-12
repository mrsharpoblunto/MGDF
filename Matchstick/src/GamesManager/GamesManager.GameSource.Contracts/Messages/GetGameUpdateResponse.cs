using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGameUpdateResponse: ResponseBase
    {
        [DataMember]
        public GameVersionUpdate LatestVersion { get; set; }
    }
}