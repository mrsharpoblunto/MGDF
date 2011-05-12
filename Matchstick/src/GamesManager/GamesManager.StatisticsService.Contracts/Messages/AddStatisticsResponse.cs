using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.StatisticsService.Contracts.Messages
{
    [DataContract]
    public class AddStatisticsResponse
    {
        private List<string> _errors = new List<string>();

        [DataMember]
        public List<string> Errors
        {
            get { return _errors; }
            set { _errors = value; }
        }
    }
}