using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class XmlEntityTests: BaseTest
    {
        [Test]
        public void LoadGame()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.xml", ReadTextFile("console.xml"));
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory,"game\\game.xml"));
            Assert.AreEqual("Console",game.Uid);
            Assert.AreEqual("Lua Console", game.Name);
            Assert.AreEqual("A Lua command console for interacting with the MGDF system", game.Description);
            Assert.AreEqual(new Version(0,1), game.Version);
            Assert.AreEqual("no-8", game.DeveloperUid);
            Assert.AreEqual("http://www.junkship.org", game.Homepage);
            Assert.AreEqual(1, game.InterfaceVersion);
            Assert.AreEqual("no8 interactive",game.DeveloperName);
            Assert.AreEqual(true, game.IsValid);
            Assert.AreEqual("http://games.junkship.org/gamesource.asmx", game.GameSourceService);
            Assert.AreEqual("http://statistics.junkship.org/statisticsservice.asmx", game.StatisticsService);
        }

        [Test]
        public void LoadInvalidGameNoUid()
        {
            const string NoUid = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>0.1</version>
  <interfaceversion>1</interfaceversion>
  <developer>no.8 Interactive</developer>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice> 
</mgdf:game>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.xml", NoUid);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidVersion()
        {
            const string InvalidVersion = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>  
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0 Beta 2</version>
  <interfaceversion>1</interfaceversion>
  <developer>no.8 Interactive</developer>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice> 
</mgdf:game>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\game"));
            gameDirectory.AddFile("game.xml", InvalidVersion);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidXML()
        {
            const string InvalidXml = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>  
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no.8 Interactive</developeruid>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory+"\\game"));
            gameDirectory.AddFile("game.xml", InvalidXml);
            Game game = new Game(Path.Combine(EnvironmentSettings.Current.AppDirectory, "game\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }
    }
}
