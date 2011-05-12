using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class AuthenticatedRequestBase: RequestBase
    {
        [DataMember]
        public string DeveloperKey { get; set; }

        [DataMember]
        public string HMAC { get; set; }

        [DataMember]
        public Guid CNonce { get; set; }

        [DataMember]
        public string Timestamp { get; set; }
    }
}
