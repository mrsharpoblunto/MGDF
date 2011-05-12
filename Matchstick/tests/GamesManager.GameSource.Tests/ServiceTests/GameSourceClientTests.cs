using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.Model.ClientModel;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Helpers;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.GameSource.Tests.ServiceTests
{
    [TestFixture]
    public class GameSourceClientTests : ServiceTestFixtureBase<GamesService>
    {
        private Guid _userId;

        protected override void InsertTestData()
        {
            Developer developer1 = new Developer
                                       {
                                           Name = "developer1",
                                           Id = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5F"),
                                           Uid = "developer1",
                                           Homepage = "http://www.example.com"
                                       };
            GameSourceRepository.Current.Insert(developer1);

            Game game1 = new Game
                             {
                                 Id = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                 Uid = "game1",
                                 Name = "game1",
                                 Description = "Test game",
                                 Homepage = "www.example.com",
                                 DeveloperId = developer1.Id,
                                 InterfaceVersion = 1,
                                 RequiresAuthentication = true
                             };
            GameSourceRepository.Current.Insert(game1);

            User user = User.Create("testuser", "Password1");
            _userId = user.Id;
            GameSourceRepository.Current.Insert(user);

            UserGame userGame = new UserGame { GameId = game1.Id, UserId = user.Id };
            GameSourceRepository.Current.Insert(userGame);

            DefaultFileServerGameData gameDataEntity = new DefaultFileServerGameData
                                                           {
                                                               GameFile = "\\developer1\\game1v1.mza",
                                                               Id = new Guid("6E779F1B-4DF2-4B25-05F4-21548F4771D7")
                                                           };
            GameSourceRepository.Current.Insert(gameDataEntity);

            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game1 version 1",
                                              GameId = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D105"),
                                              IsUpdate = false,
                                              Published = true,
                                              Version = "1.0.0.0",
                                              Md5Hash = "md51",
                                              GameDataId = gameDataEntity.Id.ToString()
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            DefaultFileServerGameData game2DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "\\developer1\\game1v1a.mza",
                                                                Id = new Guid("6E779F1B-4DF2-4B25-05F4-22548F4771D7")
                                                            };
            GameSourceRepository.Current.Insert(game2DataEntity);

            GameVersion gameVersionUpdate = new GameVersion
                                                {
                                                    CreatedDate = TimeService.Current.Now,
                                                    Description = "game1 version 1 update",
                                                    GameId = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                                    Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D107"),
                                                    IsUpdate = true,
                                                    Published = true,
                                                    UpdateMinVersion = "0.9.0.0",
                                                    UpdateMaxVersion = "1.0.1.1",
                                                    Version = "1.1.0.0",
                                                    Md5Hash = "md51a",
                                                    GameDataId = game2DataEntity.Id.ToString()
                                                };
            GameSourceRepository.Current.Insert(gameVersionUpdate);

            DefaultFileServerGameData game3DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "\\developer1\\game1v1b.mza",
                                                                Id = new Guid("6E779F1B-4DF2-4B25-05F4-22548F4771E7")
                                                            };
            GameSourceRepository.Current.Insert(game3DataEntity);

            GameVersion gameVersionUpdate2 = new GameVersion
                                                 {
                                                     CreatedDate = TimeService.Current.Now,
                                                     Description = "game1 version 1 update 2",
                                                     GameId = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                                     Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D108"),
                                                     IsUpdate = true,
                                                     Published = true,
                                                     UpdateMinVersion = "1.1.0.0",
                                                     UpdateMaxVersion = "1.1.0.0",
                                                     Version = "1.2.0.0",
                                                     Md5Hash = "md51b",
                                                     GameDataId = game3DataEntity.Id.ToString()
                                                 };
            GameSourceRepository.Current.Insert(gameVersionUpdate2);
            GameSourceRepository.Current.SubmitChanges();
        }


        [TearDown]
        public override void TearDown()
        {
            SettingsManager.Instance.Dispose();
        }

        [SetUp]
        public override void SetUp()
        {
            base.SetUp();
            EnvironmentSettings.Current = new MockEnvironmentSettings();
            EntityFactory.Current = new EntityFactory();
            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            var volumeRoot = filesystem.AddVolumeRoot("C:");
            volumeRoot.AddDirectory("temp");
            MockDirectory documentsDir = volumeRoot.AddDirectory("Documents and Settings");
            documentsDir.AddDirectory("All Users").AddDirectory("Application Data");
            MockDirectory userDir = documentsDir.AddDirectory("user");
            userDir.AddDirectory("Local Settings").AddDirectory("Application Data");

            MockDirectory schemasDir = volumeRoot.AddDirectory("program files").AddDirectory("MGDF").AddDirectory("schemas");
            schemasDir.AddFile("game.xsd", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<xs:schema 	xmlns:xs=""http://www.w3.org/2001/XMLSchema""
        	xmlns=""http://schemas.matchstickframework.org/2007/game""
        	targetNamespace=""http://schemas.matchstickframework.org/2007/game"">
	<xs:element name=""game"">
		<xs:complexType>
			<xs:all>
        <xs:element name=""gameuid"" type=""xs:string"" />
				<xs:element name=""gamename"" type=""xs:string""/>
				<xs:element name=""description"" type=""xs:string""/>
				<xs:element name=""version"" type=""xs:string""/>
        <xs:element name=""interfaceversion"" type=""xs:int""/>
        <xs:element name=""developeruid"" type=""xs:string"" />
        <xs:element name=""developername"" type=""xs:string"" />
        <xs:element name=""supportemail"" type=""xs:string"" minOccurs=""0""/>
				<xs:element name=""homepage"" type=""xs:string"" minOccurs=""0""/>
        <xs:element name=""gamesourceservice"" type=""xs:string"" minOccurs=""0"" />
        <xs:element name=""statisticsservice"" type=""xs:string"" minOccurs=""0"" />
        <xs:element name=""statisticsprivacypolicy"" type=""xs:string"" minOccurs=""0"" />
			</xs:all>
		</xs:complexType>
	</xs:element>
</xs:schema>
");
        }

        [Test]
        public void TestGameSourcemanifestHelper()
        {
            HttpRequestManager.Current = new MockHttpRequestManager();

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesourcemanifest.xml", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<mgdf:gamesourcemanifest xmlns:mgdf=""http://schemas.matchstickframework.org/2007/gamesourcemanifest""
          xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gamesourceservice>/games.svc</gamesourceservice>
  <developerservice>/developer.svc</developerservice>
</mgdf:gamesourcemanifest>");

            GameSourceManifestHelper manifestHelper = new GameSourceManifestHelper("http://games.junkship.org");
            Assert.AreEqual("http://games.junkship.org/games.svc", manifestHelper.GamesServiceUrl);
            Assert.AreEqual("http://games.junkship.org/developer.svc", manifestHelper.DeveloperServiceUrl);
        }

        [Test]
        public void TestClientFindsAvailableContiguousUpdatesNoCachedCredentials()
        {
            HttpRequestManager.Current = new MockHttpRequestManager();
            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", "testuser", "Password1");

            FileSystem.Current.GetFile("c:\\game.xml").WriteText(@"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>game1</gameuid>
  <gamename>game1</gamename>
  <description>Test game</description>
  <version>1.0.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>developer1</developeruid>
  <developername>developer1</developername>
  <homepage>www.example.com</homepage>
  <gamesourceservice>http://games.junkship.org</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            IGame game = EntityFactory.Current.CreateGame("c:\\game.xml");

            List<string> errors = new List<string>();
            GameSourceClient.ServiceFactory = source => Service;
            GameSourceClient client = new GameSourceClient();
            var updates = client.GetGameUpdates(game, args =>
                                                          {
                                                              args.UserName = "testuser";
                                                              args.Password = "Password1";
                                                              return true;
                                                          },
                                                args => true,
                                                errors);

            Assert.AreEqual(0, errors.Count);

            Assert.AreEqual(2, updates.Count);
            Assert.AreEqual("1.1.0.0", updates[0].Version);
            Assert.AreEqual("1.2.0.0", updates[1].Version);

            Assert.AreEqual(1, SettingsManager.Instance.Games.Count);
            Assert.AreEqual("game1", SettingsManager.Instance.Games[0].GameUid);
            Assert.AreEqual(true, SettingsManager.Instance.Games[0].StatisticsServiceEnabled);
            Assert.AreEqual("testuser", SettingsManager.Instance.Games[0].UserName);
            Assert.AreEqual("Password1", SettingsManager.Instance.Games[0].Password);

            //clear out the settings, ensuring they are reloaded from disk next time
            SettingsManager.Instance.Dispose();

            Assert.AreEqual(1, SettingsManager.Instance.Games.Count);
            Assert.AreEqual("game1", SettingsManager.Instance.Games[0].GameUid);
            Assert.AreEqual(true, SettingsManager.Instance.Games[0].StatisticsServiceEnabled);
            Assert.AreEqual("testuser", SettingsManager.Instance.Games[0].UserName);
            Assert.AreEqual("Password1", SettingsManager.Instance.Games[0].Password);
        }


        [Test]
        public void TestClientFindsAvailableContiguousUpdatesCachedCredentials()
        {
            SettingsManager.Instance.Games.Add(new GameSettings
                                                   {
                                                       GameUid = "game1",
                                                       UserName = "testuser",
                                                       Password = "Password1"
                                                   });

            HttpRequestManager.Current = new MockHttpRequestManager();
            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", "testuser", "Password1");

            FileSystem.Current.GetFile("c:\\game.xml").WriteText(@"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>game1</gameuid>
  <gamename>game1</gamename>
  <description>Test game</description>
  <version>1.0.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>developer1</developeruid>
  <developername>developer1</developername>
  <homepage>www.example.com</homepage>
  <gamesourceservice>http://games.junkship.org</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            IGame game = EntityFactory.Current.CreateGame("c:\\game.xml");

            List<string> errors = new List<string>();
            GameSourceClient.ServiceFactory = source => Service;
            GameSourceClient client = new GameSourceClient();
            var updates = client.GetGameUpdates(game, args => false, args=> false,errors);

            Assert.AreEqual(0, errors.Count);

            Assert.AreEqual(2, updates.Count);
            Assert.AreEqual("1.1.0.0", updates[0].Version);
            Assert.AreEqual("1.2.0.0", updates[1].Version);

            Assert.AreEqual(1, SettingsManager.Instance.Games.Count);
            Assert.AreEqual("game1", SettingsManager.Instance.Games[0].GameUid);
            Assert.AreEqual(false, SettingsManager.Instance.Games[0].StatisticsServiceEnabled);
            Assert.AreEqual("testuser", SettingsManager.Instance.Games[0].UserName);
            Assert.AreEqual("Password1", SettingsManager.Instance.Games[0].Password);
        }

        [Test]
        public void TestClientFindsAvailableContiguousUpdatesInvalidCredentials()
        {
            HttpRequestManager.Current = new MockHttpRequestManager();
            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", "testuser", "Password1");

            FileSystem.Current.GetFile("c:\\game.xml").WriteText(@"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>game1</gameuid>
  <gamename>game1</gamename>
  <description>Test game</description>
  <version>1.0.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>developer1</developeruid>
  <developername>developer1</developername>
  <homepage>www.example.com</homepage>
  <gamesourceservice>http://games.junkship.org</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            IGame game = EntityFactory.Current.CreateGame("c:\\game.xml");

            List<string> errors = new List<string>();
            GameSourceClient.ServiceFactory = source => Service;
            GameSourceClient client = new GameSourceClient();
            bool first = true;
            var updates = client.GetGameUpdates(game, args =>
                                                          {
                                                              args.UserName = "testuser";
                                                              args.Password = "wrongpassword";
                                                              if (first)
                                                              {
                                                                  first = false;
                                                                  return true;
                                                              }
                                                              return false;
                                                          },
                                                args => true,
                                                errors);

            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("No valid credentials supplied", errors[0]);

            Assert.AreEqual(0, updates.Count);

            Assert.AreEqual(1, SettingsManager.Instance.Games.Count);
            Assert.AreEqual("game1", SettingsManager.Instance.Games[0].GameUid);
            Assert.AreEqual(true, SettingsManager.Instance.Games[0].StatisticsServiceEnabled);
            Assert.AreEqual(null, SettingsManager.Instance.Games[0].UserName);
            Assert.AreEqual(null, SettingsManager.Instance.Games[0].Password);
        }

        [Test]
        public void TestClientFindsAvailableContiguousUpdatesNoCredentials()
        {
            HttpRequestManager.Current = new MockHttpRequestManager();
            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", "testuser", "Password1");

            FileSystem.Current.GetFile("c:\\game.xml").WriteText(@"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>game1</gameuid>
  <gamename>game1</gamename>
  <description>Test game</description>
  <version>1.0.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>developer1</developeruid>
  <developername>developer1</developername>
  <homepage>www.example.com</homepage>
  <gamesourceservice>http://games.junkship.org</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            IGame game = EntityFactory.Current.CreateGame("c:\\game.xml");

            List<string> errors = new List<string>();
            GameSourceClient.ServiceFactory = source => Service;
            GameSourceClient client = new GameSourceClient();

            var updates = client.GetGameUpdates(game, 
                                                args =>false,
                                                args => false,
                                                errors);

            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("No valid credentials supplied", errors[0]);

            Assert.AreEqual(0, updates.Count);

            Assert.AreEqual(1, SettingsManager.Instance.Games.Count);
            Assert.AreEqual("game1", SettingsManager.Instance.Games[0].GameUid);
            Assert.AreEqual(false, SettingsManager.Instance.Games[0].StatisticsServiceEnabled);
            Assert.AreEqual(null, SettingsManager.Instance.Games[0].UserName);
            Assert.AreEqual(null, SettingsManager.Instance.Games[0].Password);
        }
    }
}