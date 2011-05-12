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
        public List<Statistic> Statistics
        {
            get { return _statistics; }
            set { _statistics = value; }
        }
    }
}