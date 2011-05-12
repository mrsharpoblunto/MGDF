using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class UninstallGameRequest : RequestBase
    {
        [DataMember]
        public string GameUid { get; set; }
    }
}