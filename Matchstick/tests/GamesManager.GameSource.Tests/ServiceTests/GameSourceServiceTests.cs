using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using NUnit.Framework;

namespace MGDF.GamesManager.GameSource.Tests.ServiceTests
{
    [TestFixture]
    public class GameSourceServiceTests: ServiceTestFixtureBase<GamesService>
    {
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

            Developer developer2 = new Developer
                                       {
                                           Name = "developer2",
                                           Id = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5A"),
                                           Uid = "developer2",
                                           Homepage = "http://www.example.com"
                                       };
            GameSourceRepository.Current.Insert(developer2);

            Game game1 = new Game
                             {
                                 Id = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                 Uid = "game1",
                                 Name = "game1",
                                 Description = "Test game",
                                 Homepage = "www.example.com",
                                 DeveloperId = developer1.Id,
                                 InterfaceVersion = 1,
                                 RequiresAuthentication = false
                             };
            GameSourceRepository.Current.Insert(game1);

            DefaultFileServerGameData game1DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "\\developer1\\game1v1.mza",
                                                                Id = new Guid("6E679F1B-4DF2-4B25-95F4-21548F4771D7")
                                                            };
            GameSourceRepository.Current.Insert(game1DataEntity);

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
                                              GameDataId = game1DataEntity.Id.ToString()
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            DefaultFileServerGameData game2DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "\\developer1\\game1v1a.mza",
                                                                Id = new Guid("6E679F1B-4DF2-4B25-05F4-21548F4771D7")
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
                                                                Id = new Guid("6E779F1B-4DF2-4B25-05F4-21548F4771D7")
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

            Game game2 = new Game
                             {
                                 Id = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                 Uid = "game2",
                                 Name = "game2",
                                 Description = "Test game",
                                 Homepage = "www.example.com",
                                 DeveloperId = developer2.Id,
                                 InterfaceVersion = 1,
                                 RequiresAuthentication = false
                             };
            GameSourceRepository.Current.Insert(game2);

            GameSourceRepository.Current.SubmitChanges();
        }

        [Test]
        public void TestGetUpdate()
        {
            var response = Service.GetGameUpdate("1","game1","1.0.0.0");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual(new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D107"), response.LatestVersion.Id);
            Assert.AreEqual("game1 version 1 update", response.LatestVersion.Description);
            Assert.AreEqual(true, response.LatestVersion.IsUpdate);
            Assert.AreEqual(true, response.LatestVersion.Published);
            Assert.AreEqual("1.1.0.0", response.LatestVersion.Version);
            Assert.AreEqual("md51a", response.LatestVersion.Md5Hash);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", response.LatestVersion.DownloadURL);

            response = Service.GetGameUpdate("1","game1","1.1.0.0");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual(new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D108"), response.LatestVersion.Id);
            Assert.AreEqual("game1 version 1 update 2", response.LatestVersion.Description);
            Assert.AreEqual(true, response.LatestVersion.IsUpdate);
            Assert.AreEqual(true, response.LatestVersion.Published);
            Assert.AreEqual("1.2.0.0", response.LatestVersion.Version);
            Assert.AreEqual("md51b", response.LatestVersion.Md5Hash);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRCA.mza", response.LatestVersion.DownloadURL);
        }

        [Test]
        public void TestGetUpdateNoneAavailable()
        {
            var response = Service.GetGameUpdate("1","game1","0.8.0.0");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.IsNull(response.LatestVersion);

            response = Service.GetGameUpdate("1","game1","1.0.5.0");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.IsNull(response.LatestVersion);
        }

        [Test]
        public void TestGetGamesByInterfaceVersion()
        {
            var response = Service.GetGames("developer1","1");
            Assert.AreEqual(0,response.Errors.Count);
            Assert.AreEqual(1, response.Games.Count);
            Assert.AreEqual("game1", response.Games[0].Uid);
            Assert.AreEqual("game1", response.Games[0].Name);
            Assert.AreEqual("Test game", response.Games[0].Description);
            Assert.AreEqual("developer1", response.Games[0].Developer.Name);
            Assert.AreEqual("developer1", response.Games[0].Developer.Uid);
            Assert.AreEqual("http://www.example.com", response.Games[0].Developer.Homepage);
            Assert.AreEqual(false, response.Games[0].RequiresAuthentication);
            Assert.AreEqual(1, response.Games[0].InterfaceVersion);
            Assert.AreEqual("www.example.com", response.Games[0].Homepage);
            Assert.AreEqual(new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D105"), response.Games[0].LatestVersion.Id);
            Assert.AreEqual("game1 version 1", response.Games[0].LatestVersion.Description);
            Assert.AreEqual(false, response.Games[0].LatestVersion.IsUpdate);
            Assert.AreEqual(true, response.Games[0].LatestVersion.Published);
            Assert.AreEqual("1.0.0.0", response.Games[0].LatestVersion.Version);
            Assert.AreEqual("md51", response.Games[0].LatestVersion.Md5Hash);
            Assert.AreEqual("http://games.junkship.org/Downloads/k-4eNMVB8Eik19TyNLnRBQ.mza", response.Games[0].LatestVersion.DownloadURL);

            Assert.AreEqual("game2", response.Games[1].Uid);

            response = Service.GetGames("developer1", "1");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual(0, response.Games.Count);
        }

        [Test]
        public void TestGetGamesByGameUid()
        {
            var response = Service.GetGame("1","game1");
            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual(1, response.Games.Count);
            Assert.AreEqual("game1", response.Games[0].Uid);
        }
    }
}