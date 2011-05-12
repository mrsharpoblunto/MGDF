using System.Runtime.Serialization;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetOrCreateUserRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public string UserName { get; set; }

        [DataMember]
        public string CustomUserData { get; set; }

        [DataMember]
        public string PasswordHash { get; set; }
    }
}