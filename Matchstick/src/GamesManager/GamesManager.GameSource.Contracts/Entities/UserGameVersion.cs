﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class UserGameVersion: GameVersionBase
    {
        [DataMember]
        public string AuthenticatedDownloadURL { get; set; }
    }
}
