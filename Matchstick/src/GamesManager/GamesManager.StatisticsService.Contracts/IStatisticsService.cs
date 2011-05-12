using System;
using System.Collections.Generic;
using System.ServiceModel;
using System.Text;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.StatisticsService.Contracts
{
    [ServiceContract(Name = "StatisticsService",Namespace = "http://schemas.matchstickframework.org/2010")]
    public interface IStatisticsService
    {
        [OperationContract]
        AddStatisticsResponse AddStatistics(AddStatisticsRequest request);
    }
}