using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class EditGameVersionRequest : DeleteGameVersionRequest
    {
        [DataMember]
        public bool Published { get; set; }
    }
}
