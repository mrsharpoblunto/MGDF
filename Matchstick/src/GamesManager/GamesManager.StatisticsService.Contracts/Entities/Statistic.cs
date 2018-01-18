using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.StatisticsService.Contracts.Entities
{
    [DataContract]
    public class Statistic
    {
		[DataMember(Name = "timestamp")]
		public uint Timestamp { get; set; }

        [DataMember(Name = "name")]
        public string Name { get; set; }

        [DataMember(Name = "value")]
        public string Value { get; set; }
    }
}