using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.Contracts.Messages
{
    [DataContract]
    public class CheckCredentialsResponse: ResponseBase
    {
        [DataMember]
        public Developer Developer { get; set; }
    }
}
