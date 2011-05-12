using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGameUpdateReqest : RequestBase
    {
        [DataMember]
        public string GameUid { get; set; }

        [DataMember]
        public string InstalledVersion { get; set; }
    }
}