using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.Tests
{
	class MockStatistic
	{
		public string Name;
		public string Value;
		public string GameUid;
		public string SessionId;
		public uint Timestamp;
	}

	class MockStatisticsService: IStatisticsService
	{
		public List<MockStatistic> Statistics
		{
			get;
			private set;
		}

		public MockStatisticsService()
		{
			Statistics = new List<MockStatistic>();
		}

		public AddStatisticsResponse AddStatistics(AddStatisticsRequest request)
		{
			foreach (var stat in request.Statistics)
			{
				Statistics.Add(new MockStatistic {
					Name = stat.Name,
					Value = stat.Value,
					Timestamp = stat.Timestamp,
					SessionId = request.SessionId,
					GameUid = request.GameUid
				});
			}

			return new AddStatisticsResponse { Errors = new List<string>() };
		}
	}
}
