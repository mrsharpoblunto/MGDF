using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Model;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.GameSource.Tests.ServiceTests
{
    [TestFixture]
    public class StatisticsServiceTests : ServiceTestFixtureBase<MGDF.GamesManager.StatisticsService.StatisticsService>
    {
        protected override void InsertTestData()
        {
        }

        [SetUp]
        public override void SetUp()
        {
            base.SetUp();
            EnvironmentSettings.Current = new MockEnvironmentSettings();
            EntityFactory.Current = new EntityFactory();
            MockFileSystem filesystem = (MockFileSystem) FileSystem.Current;
            filesystem.AddVolumeRoot("C:");
        }

        [Test]
        public void TestStatisticsClient()
        {
            FileSystem.Current.GetFile("C:\\stats.txt").WriteText(@"key value
key1 value1
key2 value2
key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key11111112222222222 value11111value11111value11111value11111value11111value11111value11111value11111value11111value11111value11111value111112222222222
");
            StatisticsSession session = new StatisticsSession("game1","http://stats.junkship.org","c:\\stats.txt");

            StatisticsServiceClient.ServiceFactory = uri => new MockWCFClient<IStatisticsService>(Service);
            StatisticsServiceClient client = new StatisticsServiceClient(session);

            List<string> errors = new List<string>();
            client.SendStatistics(errors);

            Assert.AreEqual(0,errors.Count);

            var uploadedStats = new List<Statistic>(StatisticsServiceRepository.Current.Get<Statistic>().Where(s => s.GameUid == "game1"));
            Assert.AreEqual(4,uploadedStats.Count);
            Assert.AreEqual("key", uploadedStats[0].Name);
            Assert.AreEqual("value", uploadedStats[0].Value);
            Assert.AreEqual("key1", uploadedStats[1].Name);
            Assert.AreEqual("value1", uploadedStats[1].Value);
            Assert.AreEqual("key2", uploadedStats[2].Name);
            Assert.AreEqual("value2", uploadedStats[2].Value);
            Assert.AreEqual("key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key1111111key11111112222222", uploadedStats[3].Name);
            Assert.AreEqual("value11111value11111value11111value11111value11111value11111value11111value11111value11111value11111value11111value111112222222", uploadedStats[3].Value);
        }
    }
}