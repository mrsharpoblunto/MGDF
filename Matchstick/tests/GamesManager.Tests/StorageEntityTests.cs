using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class StorageEntityTests: BaseTest
    {
        [Test]
        public void LoadGame()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", ReadTextFile("console.json"));
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory,"game\\game.json"));
            Assert.AreEqual("Console",game.Uid);
            Assert.AreEqual("Lua Console", game.Name);
            Assert.AreEqual(new Version(0,1), game.Version);
            Assert.AreEqual("http://www.junkship.org", game.Homepage);
            Assert.AreEqual(1, game.InterfaceVersion);
            Assert.AreEqual("no8 interactive",game.DeveloperName);
            Assert.AreEqual(true, game.IsValid);
            Assert.AreEqual("http://games.junkship.org/gamesource.asmx", game.UpdateService);
            Assert.AreEqual("http://statistics.junkship.org/statisticsservice.asmx", game.StatisticsService);
        }

        [Test]
        public void LoadInvalidGameNoUid()
        {
            const string NoUid = @"{
  ""gameName"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""0.1"",
  ""interfaceVersion"":""1"",
  ""developerUid"":""no-8"",
  ""developerName"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceService"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsService"":""http://statistics.junkship.org/statisticsService.asmx"",
  ""statisticsPrivacyPolicy"":""We wont use ur informationz"",
  ""supportEmail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", NoUid);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidVersion()
        {
            const string InvalidVersion = @"{
  ""gameUid"":""Console"",
  ""gameName"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""1.0 Beta 2"",
  ""interfaceVersion"":""1"",
  ""developerUid"":""no-8"",
  ""developerName"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gameSourceService"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsService"":""http://statistics.junkship.org/statisticsService.asmx"",
  ""statisticsPrivacyPolicy"":""We wont use ur informationz"",
  ""supportEmail"":""support@junkship.org""
}";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.json", InvalidVersion);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidXML()
        {
            const string InvalidXml = @"{
  ""gameUid"":""Console"",
  ""gameName"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""1.0 Beta 2"",
  ""interfaceVersion"":""1"",
  ""developerUid"":""no-8"",
  ""developerName"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gameSourceService"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsService"":""http://statistics.junkship.org/statisticsService.asmx"",
  ""statisticsPrivacyPolicy"":""We wont use ur informationz""";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory+"\\game"));
            gameDirectory.AddFile("game.json", InvalidXml);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.json"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

		[Test]
		public void LoadDependenciesFile()
		{
			const string json = @"{
	""dependencies"":[
		{ ""command"": ""vc2013redist_x86.exe"", ""arguments"":""/q"" }
	]
}";

			MockDirectory depsDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory));
			depsDirectory = depsDirectory.AddDirectory("dependencies");
			depsDirectory.AddFile("dependencies.json", json);
			var deps = new FrameworkDependencies(Resources.DependenciesFile);
			Assert.AreEqual(true, deps.IsValid);
			Assert.AreEqual(0, deps.ErrorCollection.Count);
			Assert.AreEqual(1, deps.Dependencies.Count);
			Assert.AreEqual("/q", deps.Dependencies["vc2013redist_x86.exe"]);
		}
    }
}
