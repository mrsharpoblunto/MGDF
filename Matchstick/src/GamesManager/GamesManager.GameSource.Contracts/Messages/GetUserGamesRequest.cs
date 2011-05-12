using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetUserGamesRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public Guid UserId { get; set; }
    }
}