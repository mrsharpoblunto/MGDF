using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.StatisticsService.Contracts.Entities
{
    [DataContract]
    public class Statistic
    {
        [DataMember]
        public string Name { get; set; }

        [DataMember]
        public string Value { get; set; }

        [DataMember]
        public string GameUid { get; set; }
    }
}