using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class GetGameVersionsRequest: AuthenticatedRequestBase
    {
        [DataMember]
        public string GameUid { get; set; }
    }
}
