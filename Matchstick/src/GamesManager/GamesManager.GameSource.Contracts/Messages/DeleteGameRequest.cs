using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class DeleteGameRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public string GameUid { get; set; }
    }
}