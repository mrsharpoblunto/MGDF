using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGamesRequest : RequestBase
    {
        [DataMember]
        public string GameUid { get; set; }

        [DataMember]
        public string DeveloperUid { get; set; }

        [DataMember]
        public int InterfaceVersion { get; set; }
    }
}