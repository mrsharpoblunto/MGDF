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

		[DataMember(Name = "gameUid")]
		public string GameUid { get; set; }

		[DataMember(Name = "sessionId")]
		public string SessionId { get; set; }

        [DataMember(Name = "statistics")]
        public List<Statistic> Statistics
        {
            get { return _statistics; }
            set { _statistics = value; }
        }
    }
}