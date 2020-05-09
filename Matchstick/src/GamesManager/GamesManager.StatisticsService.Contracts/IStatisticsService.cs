using System;
using System.Collections.Generic;
using System.ServiceModel;
using System.ServiceModel.Web;
using System.Text;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.StatisticsService.Contracts
{
  [ServiceContract(Name = "StatisticsService")]
  public interface IStatisticsService
  {
    [OperationContract]
    [WebInvoke(RequestFormat = WebMessageFormat.Json, UriTemplate = "/statistics", ResponseFormat = WebMessageFormat.Json, BodyStyle = WebMessageBodyStyle.Bare, Method = "POST")]
    AddStatisticsResponse AddStatistics(AddStatisticsRequest request);
  }
}