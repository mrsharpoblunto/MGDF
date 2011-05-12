using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.ServiceModel;
using System.ServiceModel.Activation;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;
using MGDF.GamesManager.StatisticsService.Model;

namespace MGDF.GamesManager.StatisticsService
{
    public class StatisticsService : IStatisticsService 
    {
        public AddStatisticsResponse AddStatistics(AddStatisticsRequest request)
        {
            AddStatisticsResponse response = new AddStatisticsResponse();
            try
            {
                foreach (var statistic in request.Statistics)
                {
                    Statistic stat = new Statistic
                                         {
                                             Id = Guid.NewGuid(),
                                             GameUid = statistic.GameUid,
                                             Name = statistic.Name,
                                             Value = statistic.Value
                                         };

                    StatisticsServiceRepository.Current.Insert(stat);
                }
                StatisticsServiceRepository.Current.SubmitChanges();
            }
            catch (Exception ex)
            {
                response.Errors.Add("Unknown error");
                Logger.Current.Write(ex, "Error processing " + typeof(AddStatisticsRequest).Name);
            }

            return response;
        }
    }
}