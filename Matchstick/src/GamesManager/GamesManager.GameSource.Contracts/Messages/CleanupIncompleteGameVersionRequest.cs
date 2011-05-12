using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class CleanupIncompleteGameVersionRequest:AuthenticatedRequestBase
    {
        [DataMember]
        public Guid FragmentId { get; set; }
    }
}
