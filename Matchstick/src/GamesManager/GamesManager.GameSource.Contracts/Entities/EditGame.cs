﻿using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class EditGame: GameBase
    {
        [DataMember]
        public bool RequiresAuthenticationSpecified { get; set; }
    }
}