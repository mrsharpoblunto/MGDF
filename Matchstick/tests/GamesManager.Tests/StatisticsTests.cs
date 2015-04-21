using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class StatisticsTests: BaseTest
    {

        [SetUp]
        public virtual void SetUp()
        {
            Config.Current = new MockConfig();
            TimeService.Current = new MockTimeService(new DateTime(2009, 10, 10));
            FileSystem.Current = new MockFileSystem();
            Logger.Current = new MockLogger();
            SettingsManager.Dispose();
            Resources.UninitUserDirectory();
            base.Setup();
        }

        [Test]
        public void TestStatisticsManager()
        {
            FileSystem.Current.GetFile("C:\\stats.txt").WriteText(@"0:key value
1223:key1 value1
122444:key2 value2
122445:"+new String('a',256)+" "+new String('b',256)+@"
");
            StatisticsSession session = new StatisticsSession("game1", "http://stats.junkship.org", "c:\\stats.txt");

			var statisticsService = new MockStatisticsService();
			StatisticsServiceClient.ServiceFactory = ()=>new MockWCFClient<IStatisticsService>(statisticsService);
            StatisticsServiceClient client = new StatisticsServiceClient(session);

            List<string> errors = new List<string>();
            client.SendStatistics(errors);

            Assert.AreEqual(0, errors.Count);

			var uploadedStats = statisticsService.Statistics;
            Assert.AreEqual(4, uploadedStats.Count);
            Assert.AreEqual("key", uploadedStats[0].Name);
            Assert.AreEqual("value", uploadedStats[0].Value);
			Assert.AreEqual(0, uploadedStats[0].Timestamp);
            Assert.AreEqual("key1", uploadedStats[1].Name);
            Assert.AreEqual("value1", uploadedStats[1].Value);
			Assert.AreEqual(1223, uploadedStats[1].Timestamp);
            Assert.AreEqual("key2", uploadedStats[2].Name);
            Assert.AreEqual("value2", uploadedStats[2].Value);
			Assert.AreEqual(122444, uploadedStats[2].Timestamp);
			Assert.AreEqual(new String('a', 255), uploadedStats[3].Name);
			Assert.AreEqual(new String('b', 255), uploadedStats[3].Value);
			Assert.AreEqual(122445, uploadedStats[3].Timestamp);
			Assert.AreEqual(uploadedStats[0].SessionId, uploadedStats[3].SessionId);
			Assert.IsTrue(!string.IsNullOrEmpty(uploadedStats[0].SessionId));
        }

        [Test]
        public void TestCanSendStatisticsDeniedNoPrivacyPolicy()
        {
            const string gameContent = @"{
  ""gameuid"":""Console"",
  ""gamename"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""0.1"",
  ""interfaceversion"":""1"",
  ""developeruid"":""no-8"",
  ""developername"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceservice"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsservice"":""http://statistics.junkship.org/statisticsservice.asmx"",
  ""supportemail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", gameContent);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));

            Assert.IsFalse(StatisticsSession.CanSendStatistics(game));        
        }

        [Test]
        public void TestCanSendStatistics()
        {
            const string gmeContent = @"{
  ""gameuid"":""Console"",
  ""gamename"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""0.1"",
  ""interfaceversion"":""1"",
  ""developeruid"":""no-8"",
  ""developername"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceservice"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsservice"":""http://statistics.junkship.org/statisticsservice.asmx"",
  ""statisticsprivacypolicy"":""We wont use ur informationz"",
  ""supportemail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", gmeContent);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));

            Assert.IsTrue(StatisticsSession.CanSendStatistics(game));
        }

        [Test]
        public void TestGetStatisticsPermissionCachedPermission()
        {
            const string gmeContent = @"{
  ""gameuid"":""Console"",
  ""gamename"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""0.1"",
  ""interfaceversion"":""1"",
  ""developeruid"":""no-8"",
  ""developername"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceservice"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsservice"":""http://statistics.junkship.org/statisticsservice.asmx"",
  ""statisticsprivacypolicy"":""http://www.junkship.org/privacy"",
  ""supportemail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", gmeContent);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));

            Resources.InitUserDirectory("Console", false);
            SettingsManager.Instance.Settings = new GameSettings();
            SettingsManager.Instance.Settings.GameUid = "Console";
            SettingsManager.Instance.Settings.StatisticsServiceEnabled = true;
            SettingsManager.Instance.Save();

            Assert.IsTrue(StatisticsSession.GetStatisticsPermission(game, args =>
                                                               {
                                                                   Assert.Fail("This get permission callback shouldn't be called when the permission has been cached");
                                                                   return false;
                                                               }));

        }

        [TestCase(false)]
        [TestCase(true)]
        public void TestGetStatisticsPermissionNoCachedPermission(bool allow)
        {
            const string gmeContent = @"{
  ""gameuid"":""Console"",
  ""gamename"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""0.1"",
  ""interfaceversion"":""1"",
  ""developeruid"":""no-8"",
  ""developername"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceservice"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsservice"":""http://statistics.junkship.org/statisticsservice.asmx"",
  ""statisticsprivacypolicy"":""http://www.junkship.org/privacy"",
  ""supportemail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", gmeContent);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));

            Resources.InitUserDirectory("Console", false);

            Assert.AreEqual(allow,StatisticsSession.GetStatisticsPermission(game, args => allow));

            Assert.AreEqual(SettingsManager.Instance.Settings.GameUid,"Console");
            Assert.AreEqual(allow,SettingsManager.Instance.Settings.StatisticsServiceEnabled.Value);
        }
    }
}
