using System;
using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class EditUserRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public Guid UserId { get; set; }

        [DataMember]
        public string CustomUserData { get; set; }

        [DataMember]
        public string OldPasswordHash { get; set; }

        [DataMember]
        public string NewPasswordHash { get; set; }
    }
}