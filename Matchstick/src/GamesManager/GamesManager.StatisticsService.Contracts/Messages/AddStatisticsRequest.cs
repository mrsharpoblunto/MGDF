using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.StatisticsService.Contracts.Entities;

namespace MGDF.GamesManager.StatisticsService.Contracts.Messages
{
    [DataContract]
    public class AddStatisticsRequest
    {
        private List<Statistic> _statistics = new List<Statistic>();

		[DataMember]
		public string GameUid { get; set; }

		[DataMember]
		public string SessionId { get; set; }

        [DataMember]
        public List<Statistic> Statistics
        {
            get { return _statistics; }
            set { _statistics = value; }
        }
    }
}