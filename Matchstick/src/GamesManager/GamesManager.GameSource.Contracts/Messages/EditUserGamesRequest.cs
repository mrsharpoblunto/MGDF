using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class EditUserGamesRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public Guid UserId { get; set; }

        [DataMember]
        public string GameUid { get; set; }

        [DataMember]
        public bool AllowAccess { get; set; }
    }
}