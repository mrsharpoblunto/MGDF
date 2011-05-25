﻿using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class Developer
    {
        [DataMember]
        public string Uid { get; set; }

        [DataMember]
        public string Name { get; set; }

        [DataMember]
        public string Homepage { get; set; }
    }
}