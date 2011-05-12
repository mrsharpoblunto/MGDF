using System;
using System.Runtime.Serialization;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetOrCreateUserResponse: ResponseBase
    {
        [DataMember]
        public User User { get; set; }
    }
}