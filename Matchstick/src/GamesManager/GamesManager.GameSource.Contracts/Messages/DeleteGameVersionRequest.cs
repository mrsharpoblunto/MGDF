using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Messages;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class DeleteGameVersionRequest:AuthenticatedRequestBase
    {
        [DataMember]
        public Guid Id { get; set; }
    }
}