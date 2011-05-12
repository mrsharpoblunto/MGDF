using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.StatisticsService.Model;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockStatisticsServiceRepository: MockRepositoryBase
    {
        public MockStatisticsServiceRepository(): base(new[]{ typeof(Statistic)})
        {
        }
    }
}