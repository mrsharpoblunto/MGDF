using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class SubmitStatisticsRequest:RequestBase
    {
        [DataMember]
        public string GameUid { get; set;  }

        [DataMember]
        public string StatisticsFile { get; set; }
    }
}
