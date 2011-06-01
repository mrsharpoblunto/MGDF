using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;
using Developer=MGDF.GamesManager.GameSource.Model.Developer;
using Game=MGDF.GamesManager.GameSource.Model.Game;
using GameVersion=MGDF.GamesManager.GameSource.Model.GameVersion;
using UserGame=MGDF.GamesManager.GameSource.Model.UserGame;
using User = MGDF.GamesManager.GameSource.Model.User;

namespace MGDF.GamesManager.GameSource.Tests.ServiceTests
{
    [TestFixture]
    public class GameSourceDeveloperServiceTests : ServiceTestFixtureBase<DeveloperService>
    {
        private Developer _developer1;
        private Developer _developer2;

        protected override void InsertTestData()
        {
            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games");

            _developer1 = new Developer
                              {
                                  Name = "developer 1",
                                  Id = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5F"),
                                  Uid = "developer1",
                                  Homepage = "http://www.example.com"
                              };
            _developer1.GenerateDeveloperKeySecretKeyPair();
            GameSourceRepository.Current.Insert(_developer1);

            _developer2 = new Developer
                              {
                                  Name = "developer 2",
                                  Id = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5A"),
                                  Uid = "developer2",
                                  Homepage = "http://www.example.com"
                              };
            _developer2.GenerateDeveloperKeySecretKeyPair();
            GameSourceRepository.Current.Insert(_developer2);

            Game game1 = new Game
                             {
                                 Id = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                 Uid = "game1",
                                 Name = "game1",
                                 Description = "Test game",
                                 Homepage = "www.example.com",
                                 DeveloperId = _developer1.Id,
                                 InterfaceVersion = 1,
                                 RequiresAuthentication = false
                             };
            GameSourceRepository.Current.Insert(game1);

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
                                                                Id = new Guid("6E779F1B-4DF2-1B25-05F4-21548F4771D7")
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
                                                                Id = new Guid("6E779F1B-4DF2-1B25-09F4-21548F4771D7")
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
                                 DeveloperId = _developer2.Id,
                                 InterfaceVersion = 1,
                                 RequiresAuthentication = false
                             };
            GameSourceRepository.Current.Insert(game2);

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer1.DeveloperKey;
            Settings.Instance.SecretKey = _developer1.SecretKey;

            GameSourceRepository.Current.SubmitChanges();
        }

        [Test]
        public void TestCheckCredentials()
        {
            var request = RequestBuilder.Build<AuthenticatedRequestBase>();
            var response = Service.CheckCredentials(request);

            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual("developer1",response.Developer.Uid);
            Assert.AreEqual("developer 1", response.Developer.Name);
            Assert.AreEqual("http://www.example.com", response.Developer.Homepage);

            //invalid developer key
            Developer d = new Developer();
            d.GenerateDeveloperKeySecretKeyPair();
            Settings originalSettings = Settings.Instance;
            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = d.DeveloperKey;
            Settings.Instance.SecretKey = originalSettings.SecretKey;

            request = RequestBuilder.Build<AuthenticatedRequestBase>();
            response = Service.CheckCredentials(request);
            Assert.AreEqual(1, response.Errors.Count);

            //invalid secret key
            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = originalSettings.DeveloperKey;
            Settings.Instance.SecretKey = d.SecretKey;

            request = RequestBuilder.Build<AuthenticatedRequestBase>();
            response = Service.CheckCredentials(request);
            Assert.AreEqual(1, response.Errors.Count);

            //out of date timestamp
            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = originalSettings.DeveloperKey;
            Settings.Instance.SecretKey = originalSettings.SecretKey;

            request = RequestBuilder.Build<AuthenticatedRequestBase>();
            ((MockTimeService)TimeService.Current).Now = TimeService.Current.Now.AddMinutes(16);
            response = Service.CheckCredentials(request);
            Assert.AreEqual(1, response.Errors.Count);
        }

        [Test]
        public void TestCNoncePreventsRequestReplays()
        {
            var request = RequestBuilder.Build<AuthenticatedRequestBase>();
            var response = Service.CheckCredentials(request);

            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual("developer1", response.Developer.Uid);
            Assert.AreEqual("developer 1", response.Developer.Name);
            Assert.AreEqual("http://www.example.com", response.Developer.Homepage);

            response = Service.CheckCredentials(request);
            Assert.AreEqual(1, response.Errors.Count);
        }

        [Test]
        public void TestCleanupIncompleteGameVersionMultipleVersions()
        {
            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game1 version 3",
                                              GameId = new Guid("04A4B95C-A649-439E-A0F1-94147200BE3E"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D1A5"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.3.0.0",
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            DefaultFileServerGameData fragmentDataEntity = new DefaultFileServerGameData
                                                               {
                                                                   GameFile = "/dev/gameversion3.fragment",
                                                                   Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                               };
            GameSourceRepository.Current.Insert(fragmentDataEntity);

            var gameFragment = new GameFragment
                                   {
                                       DeveloperId = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5F"),
                                       GameDataId = fragmentDataEntity.Id.ToString(),
                                       GameVersionId = gameVersion.Id,
                                       Id = new Guid("6C1074A3-FAF7-4DA2-98AA-AC6B527B721B"),
                                       PublishOnComplete = true,
                                   };
            GameSourceRepository.Current.Insert(gameFragment);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("dev").AddFile("gameversion3.fragment", "file content");

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer1.DeveloperKey;
            Settings.Instance.SecretKey = _developer1.SecretKey;
            var request = RequestBuilder.Build<CleanupIncompleteGameVersionRequest>();
            request.FragmentId = gameFragment.Id;
            Service.CleanupIncompleteGameVersion(request);

            var fragment = GameSourceRepository.Current.Get<GameFragment>().SingleOrDefault(gf => gf.Id == gameFragment.Id);
            var version = GameSourceRepository.Current.Get<GameVersion>().SingleOrDefault(gv => gv.Id == gameVersion.Id);
            var game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Id == gameVersion.GameId);
            Assert.IsNull(fragment);
            Assert.IsNull(version);
            Assert.IsNotNull(game);

            var fragmentFile = FileSystem.Current.GetFile("C:\\gamesource\\dev\\gameversion3.fragment");
            Assert.IsFalse(fragmentFile.Exists);
        }

        [Test]
        public void TestCleanupIncompleteGameVersionOnlyVersion()
        {
            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D1A5"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.0.0.0",
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            DefaultFileServerGameData fragmentDataEntity = new DefaultFileServerGameData
                                                               {
                                                                   GameFile = "/dev/gameversion1.fragment",
                                                                   Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                               };
            GameSourceRepository.Current.Insert(fragmentDataEntity);

            var gameFragment = new GameFragment
                                   {
                                       DeveloperId = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5A"),
                                       GameDataId = fragmentDataEntity.Id.ToString(),
                                       GameVersionId = gameVersion.Id,
                                       Id = new Guid("6C1074A3-FAF7-4DA2-98AA-AC6B527B721B"),
                                       PublishOnComplete = true,
                                   };
            GameSourceRepository.Current.Insert(gameFragment);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("dev").AddFile("gameversion1.fragment","file content");

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;
            var request = RequestBuilder.Build<CleanupIncompleteGameVersionRequest>();
            request.FragmentId = gameFragment.Id;
            Service.CleanupIncompleteGameVersion(request);

            var fragment = GameSourceRepository.Current.Get<GameFragment>().SingleOrDefault(gf => gf.Id == gameFragment.Id);
            var version = GameSourceRepository.Current.Get<GameVersion>().SingleOrDefault(gv => gv.Id == gameVersion.Id);
            var data = GameSourceRepository.Current.Get<PendingDelete>().SingleOrDefault(g => g.GameDataId == gameFragment.GameDataId);
            Assert.IsNull(fragment);
            Assert.IsNull(version);
            Assert.IsNotNull(data);
        }

        [Test]
        public void TestCleanupIncompleteGameVersionIncorrectDeveloper()
        {
            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D1A5"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.0.0.0",
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            DefaultFileServerGameData fragmentDataEntity = new DefaultFileServerGameData
                                                               {
                                                                   GameFile = "/dev/gameversion1.fragment",
                                                                   Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                               };
            GameSourceRepository.Current.Insert(fragmentDataEntity);

            var gameFragment = new GameFragment
                                   {
                                       DeveloperId = new Guid("6A1FA6D3-1E40-4534-857A-4646EF2B6B5A"),
                                       GameDataId = fragmentDataEntity.Id.ToString(),
                                       GameVersionId = gameVersion.Id,
                                       Id = new Guid("6C1074A3-FAF7-4DA2-98AA-AC6B527B721B"),
                                       PublishOnComplete = true,
                                   };
            GameSourceRepository.Current.Insert(gameFragment);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("dev").AddFile("gameversion1.fragment", "file content");

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer1.DeveloperKey;
            Settings.Instance.SecretKey = _developer1.SecretKey;
            var request = RequestBuilder.Build<CleanupIncompleteGameVersionRequest>();
            request.FragmentId = gameFragment.Id;
            var response = Service.CleanupIncompleteGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied",response.Errors.First().Code);
        }

        [Test]
        public void TestAddGameVersionIncorrectDeveloper()
        {
            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied", response.Errors.First().Code);
        }

        [Test]
        public void TestAddGameVersion()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(0, response.Errors.Count);

            GameFragment fragment = GameSourceRepository.Current.Get<GameFragment>().SingleOrDefault(gf => gf.Id == response.GameFragmentId);
            Assert.IsNotNull(fragment);

            Assert.AreEqual(_developer1.Id,fragment.DeveloperId);
        }

        [Test]
        public void TestCantAddDuplicateGameVersionUpdates()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             UpdateMinVersion = "1.0.0.0",
                                             UpdateMaxVersion = "1.0.0.0",
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(0, response.Errors.Count);

            GameFragment fragment = GameSourceRepository.Current.Get<GameFragment>().SingleOrDefault(gf => gf.Id == response.GameFragmentId);
            Assert.IsNotNull(fragment);

            Assert.AreEqual(_developer1.Id, fragment.DeveloperId);

            //can't add the same version update after its already been added.
            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
            {
                Description = "blah",
                IsUpdate = true,
                UpdateMinVersion = "1.0.0.0",
                UpdateMaxVersion = "1.0.0.0",
                Version = "1.3.0.0",
                Md5Hash = "md51"
            };
            response = Service.AddGameVersion(request);
            Assert.AreEqual(1, response.Errors.Count);

            //however if we alter the versions it updates from then an update that ends up with the same version is okay.
            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             UpdateMinVersion = "1.0.0.1",
                                             UpdateMaxVersion = "1.0.0.2",
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };

            response = Service.AddGameVersion(request);
            Assert.AreEqual(0, response.Errors.Count);        
        }

        [Test]
        public void TestCantAddDuplicateGameVersions()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(0, response.Errors.Count);

            GameFragment fragment = GameSourceRepository.Current.Get<GameFragment>().SingleOrDefault(gf => gf.Id == response.GameFragmentId);
            Assert.IsNotNull(fragment);

            Assert.AreEqual(_developer1.Id, fragment.DeveloperId);

            //can't add the same version after its already been added.
            response = Service.AddGameVersion(request);
            Assert.AreEqual(1,response.Errors.Count);
        }

        [TestCase("core")]
        [TestCase("downloads")]
        public void TestAddGameVersionReservedUid(string reservedUid)
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = reservedUid;
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "reservedUid",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);
        }

        [Test]
        public void TestAddGameVersionVersionInvalidGameUid()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "gamethatdoesntexist";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.1.0.0",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);
        }

        [Test]
        public void TestAddGameVersionVersionInvalidGameVersion()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "111",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);
        }

        [Test]
        public void TestAddGameVersionVersionNoUpdateVersions()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             Version = "111",
                                             UpdateMaxVersion = "111",
                                             UpdateMinVersion = "222",
                                             Md5Hash = "md51"
                                         };
            var response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             Version = "111",
                                             UpdateMinVersion = "222",
                                             Md5Hash = "md51"
                                         };
            response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             Version = "111",
                                             UpdateMaxVersion = "222",
                                             Md5Hash = "md51"
                                         };
            response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             Version = "1.0.0.1",
                                             UpdateMaxVersion = "1.0.0.0",
                                             UpdateMinVersion = "1.1.0.0",
                                             Md5Hash = "md51"
                                         };
            response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = true,
                                             Version = "1.2.0.1",
                                             UpdateMaxVersion = "1.0.0.0",
                                             UpdateMinVersion = "1.1.0.0",
                                             Md5Hash = "md51"
                                         };
            response = Service.AddGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);
        } 

        [Test]
        public void TestGetGameVersions()
        {
            var request = RequestBuilder.Build<GetGameVersionsRequest>();
            request.GameUid = "game1";
            var response = Service.GetGameVersions(request);

            Assert.AreEqual(3,response.GameVersions.Count);

            Assert.IsFalse(response.GameVersions[0].IsUpdate);
            Assert.IsTrue(response.GameVersions[0].Published);
            Assert.AreEqual("1.0.0.0",response.GameVersions[0].Version);
            Assert.AreEqual("game1 version 1", response.GameVersions[0].Description);
            Assert.AreEqual("md51", response.GameVersions[0].Md5Hash);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBQ.mza", response.GameVersions[0].DownloadURL);

            Assert.IsTrue(response.GameVersions[1].IsUpdate);
            Assert.IsTrue(response.GameVersions[1].Published);
            Assert.AreEqual("1.1.0.0", response.GameVersions[1].Version);
            Assert.AreEqual("0.9.0.0", response.GameVersions[1].UpdateMinVersion);
            Assert.AreEqual("1.0.1.1", response.GameVersions[1].UpdateMaxVersion);
            Assert.AreEqual("game1 version 1 update", response.GameVersions[1].Description);
            Assert.AreEqual("md51a", response.GameVersions[1].Md5Hash);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBw.mza", response.GameVersions[1].DownloadURL);

            Assert.IsTrue(response.GameVersions[2].IsUpdate);
            Assert.IsTrue(response.GameVersions[2].Published);
            Assert.AreEqual("1.2.0.0", response.GameVersions[2].Version);
            Assert.AreEqual("1.1.0.0", response.GameVersions[2].UpdateMinVersion);
            Assert.AreEqual("1.1.0.0", response.GameVersions[2].UpdateMaxVersion);
            Assert.AreEqual("game1 version 1 update 2", response.GameVersions[2].Description);
            Assert.AreEqual("md51b", response.GameVersions[2].Md5Hash);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRCA.mza", response.GameVersions[2].DownloadURL);
        }

        [Test]
        public void TestAddGame()
        {
            var request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      Name = "new game",
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame"
                                  };
            var response = Service.AddGame(request);
            Assert.AreEqual(0,response.Errors.Count);

            Game game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Uid == "newgame");
            Assert.IsNotNull(game);
            Assert.AreEqual("new game desc",game.Description);
            Assert.AreEqual(_developer1.Id, game.DeveloperId);
            Assert.AreEqual("http://www.example.com", game.Homepage);
            Assert.AreEqual(1, game.InterfaceVersion);
            Assert.AreEqual("new game", game.Name);
            Assert.IsFalse(game.RequiresAuthentication);
            Assert.AreEqual("newgame", game.Uid);
        }

        [Test]
        public void TestAddInvalidGame()
        {
            var request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = null;
            var response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      Name = "new game",
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                  };
            response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                     Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame"
                                  };
            response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame",
                                      Name = "new game"
                                  };
            response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);


            request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 0,
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame",
                                      Name = "new game"
                                  };
            response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);

            request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "game1",
                                      Name = "new game"
                                  };
            response = Service.AddGame(request);
            Assert.AreEqual("InvalidArguments", response.Errors.First().Code);
        }

        [Test]
        public void TestEditGameIncorrectDeveloper()
        {
            var request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      Name = "new game",
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame"
                                  };
            Service.AddGame(request);

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;

            var editRequest = RequestBuilder.Build<EditGameRequest>();
            editRequest.EditGame = new EditGame
                                       {
                                           Description = "new game desc1",
                                           Homepage = "http://www.example1.com",
                                           InterfaceVersion = 2,
                                           Name = "new game1",
                                           RequiresAuthentication = true,
                                           RequiresAuthenticationSpecified = true,
                                           Uid = "newgame"
                                       };
            var response = Service.EditGame(editRequest);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied", response.Errors.First().Code);
        }

        [Test]
        public void TestEditGame()
        {
            var request = RequestBuilder.Build<AddGameRequest>();
            request.NewGame = new EditGame
                                  {
                                      Description = "new game desc",
                                      Homepage = "http://www.example.com",
                                      InterfaceVersion = 1,
                                      Name = "new game",
                                      RequiresAuthentication = false,
                                      RequiresAuthenticationSpecified = true,
                                      Uid = "newgame"
                                  };
            Service.AddGame(request);

            var editRequest = RequestBuilder.Build<EditGameRequest>();
            editRequest.EditGame = new EditGame
                                       {
                                           Description = "new game desc1",
                                           Homepage = "http://www.example1.com",
                                           InterfaceVersion = 2,
                                           Name = "new game1",
                                           RequiresAuthentication = true,
                                           RequiresAuthenticationSpecified = true,
                                           Uid = "newgame"
                                       };
            var response = Service.EditGame(editRequest);
            Assert.AreEqual(0, response.Errors.Count);

            Game game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Uid == "newgame");
            Assert.IsNotNull(game);
            Assert.AreEqual("new game desc1", game.Description);
            Assert.AreEqual(_developer1.Id, game.DeveloperId);
            Assert.AreEqual("http://www.example1.com", game.Homepage);
            Assert.AreEqual(2, game.InterfaceVersion);
            Assert.AreEqual("new game1", game.Name);
            Assert.IsTrue(game.RequiresAuthentication);
            Assert.AreEqual("newgame", game.Uid);
        }

        [Test]
        public void TestEditGameVersion()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51",
                                             Published = false
                                         };
            Service.AddGameVersion(request);

            GameVersion version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Version == "1.3.0.0");
            Assert.IsFalse(version.Published);

            var editRequest = RequestBuilder.Build<EditGameVersionRequest>();
            editRequest.Id = version.Id;
            editRequest.Published = true;
            var response = Service.EditGameVersion(editRequest);
            Assert.AreEqual(0, response.Errors.Count);

            version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Version == "1.3.0.0");
            Assert.IsTrue(version.Published);
        }

        [Test]
        public void TestEditGameVersionIncorrectDeveloper()
        {
            var request = RequestBuilder.Build<AddGameVersionRequest>();
            request.GameUid = "game1";
            request.NewGameVersion = new GameVersionBase
                                         {
                                             Description = "blah",
                                             IsUpdate = false,
                                             Version = "1.3.0.0",
                                             Md5Hash = "md51",
                                             Published = false
                                         };
            Service.AddGameVersion(request);

            GameVersion version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Version == "1.3.0.0");
            Assert.IsFalse(version.Published);

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;

            var editRequest = RequestBuilder.Build<EditGameVersionRequest>();
            editRequest.Id = version.Id;
            editRequest.Published = true;
            var response = Service.EditGameVersion(editRequest);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied", response.Errors.First().Code);
        }

        [Test]
        public void TestDeleteGameVersion()
        {
            DefaultFileServerGameData gameDataEntity = new DefaultFileServerGameData
                                                           {
                                                               GameFile = "/games/dev/game1.mza",
                                                               Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                           };
            GameSourceRepository.Current.Insert(gameDataEntity);

            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D115"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.5.0.0",
                                              GameDataId = gameDataEntity.Id.ToString(),
                                              Md5Hash = "md5"
                                          };
            GameSourceRepository.Current.Insert(gameVersion);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games").AddDirectory("dev").AddFile("gameversion1.mza", "file content");

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;

            var request = RequestBuilder.Build<DeleteGameVersionRequest>();
            request.Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D115");
            var response = Service.DeleteGameVersion(request);
            Assert.AreEqual(0,response.Errors.Count);

            var version = GameSourceRepository.Current.Get<GameVersion>().SingleOrDefault(gv => gv.Id == gameVersion.Id);
            Assert.IsNull(version);

            var fragmentFile = FileSystem.Current.GetFile("C:\\gamesource\\games\\dev\\gameversion1.mza");
            Assert.IsFalse(fragmentFile.Exists);
        }

        [Test]
        public void TestDeleteGameVersionIncorrectDeveloper()
        {
            DefaultFileServerGameData gameDataEntity = new DefaultFileServerGameData
                                                           {
                                                               GameFile = "/games/dev/game1.mza",
                                                               Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                           };
            GameSourceRepository.Current.Insert(gameDataEntity);

            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D112"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.5.0.0",
                                              GameDataId = gameDataEntity.Id.ToString(),
                                              Md5Hash = "md5"
                                          };
            GameSourceRepository.Current.Insert(gameVersion);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games").AddDirectory("dev").AddFile("gameversion1.mza", "file content");

            var request = RequestBuilder.Build<DeleteGameVersionRequest>();
            request.Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D112");
            var response = Service.DeleteGameVersion(request);

            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied", response.Errors.First().Code);
        }

        [Test]
        public void TestDeleteGame()
        {
            DefaultFileServerGameData gameDataEntity = new DefaultFileServerGameData
                                                           {
                                                               GameFile = "/games/dev/game1.mza",
                                                               Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                           };
            GameSourceRepository.Current.Insert(gameDataEntity);

            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D112"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.5.0.0",
                                              GameDataId = gameDataEntity.Id.ToString(),
                                              Md5Hash = "md5"
                                          };
            GameSourceRepository.Current.Insert(gameVersion);

            User user = new User
                            {
                                Id = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F"),
                                Name = "jim",
                                PasswordHash = "password".ComputeMD5()
                            };
            GameSourceRepository.Current.Insert(user);

            UserGame usergame = new UserGame
                                    {
                                        Id = new Guid("DB6798D9-AB1F-482E-BD46-D288B05FF33F"),
                                        GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                        UserId = user.Id
                                    };
            GameSourceRepository.Current.Insert(usergame);

            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games").AddDirectory("dev").AddFile("gameversion1.mza", "file content");

            Settings.Instance = new Settings();
            Settings.Instance.DeveloperKey = _developer2.DeveloperKey;
            Settings.Instance.SecretKey = _developer2.SecretKey;

            var request = RequestBuilder.Build<DeleteGameRequest>();
            request.GameUid = "game2";
            var response = Service.DeleteGame(request);
            Assert.AreEqual(0, response.Errors.Count);

            //check everything is gone.
            var versions = GameSourceRepository.Current.Get<GameVersion>().Where(gv => gv.GameId == new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"));
            Assert.AreEqual(0,versions.Count());
            var usergames = GameSourceRepository.Current.Get<UserGame>().Where(ug => ug.GameId == new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"));
            Assert.AreEqual(0, usergames.Count());
            var game = GameSourceRepository.Current.Get<Game>().SingleOrDefault(g => g.Uid == "game2");
            Assert.IsNull(game);
            var versionFile = FileSystem.Current.GetFile("C:\\gamesource\\games\\dev\\gameversion1.mza");
            Assert.IsFalse(versionFile.Exists);
        }

        [Test]
        public void TestCreateGetAndEditUser()
        {
            var request = RequestBuilder.Build<GetOrCreateUserRequest>();
            request.UserName = "newuser";
            request.PasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password1");
            request.CustomUserData = "blah";

            var response = Service.GetOrCreateUser(request);
            Assert.AreEqual(0,response.Errors.Count);
            Guid userId = response.User.Id;

            //was a new user created
            User user = GameSourceRepository.Current.Get<User>().SingleOrDefault(u => u.Name == "newuser");
            Assert.AreEqual("newuser",user.Name);
            Assert.AreEqual(request.PasswordHash, user.PasswordHash);

            //check that calling again results in the existing user being retrieved, not a new one created.
            request = RequestBuilder.Build<GetOrCreateUserRequest>();
            request.UserName = "newuser";
            request.PasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password1");
            request.CustomUserData = "blah";
            response = Service.GetOrCreateUser(request);

            Assert.AreEqual(0, response.Errors.Count);
            Assert.AreEqual(response.User.Id,userId);
            Assert.AreEqual(response.User.CustomUserData, "blah");

            int userCount = GameSourceRepository.Current.Get<User>().Count(u => u.Name == "newuser");
            Assert.AreEqual(1,userCount);
            Assert.AreEqual(request.PasswordHash, user.PasswordHash);

            //now try to create a user with the same name but different password (will fail)
            request = RequestBuilder.Build<GetOrCreateUserRequest>();
            request.UserName = "newuser";
            request.CustomUserData = "blah";
            request.PasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password2");
            response = Service.GetOrCreateUser(request);

            Assert.AreEqual(1, response.Errors.Count);

            //change the users password (incorrect old password. will fail)
            var editRequest = RequestBuilder.Build<EditUserRequest>();
            editRequest.UserId = userId;
            editRequest.OldPasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password3");
            editRequest.NewPasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password2");
            var editResponse = Service.EditUser(editRequest);

            Assert.AreEqual(1, editResponse.Errors.Count);

            //change the users password
            editRequest = RequestBuilder.Build<EditUserRequest>();
            editRequest.UserId = userId;
            editRequest.OldPasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password1");
            editRequest.NewPasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password2");
            editResponse = Service.EditUser(editRequest);

            Assert.AreEqual(0,editResponse.Errors.Count);

            //now try to get a user with the same name but new password
            request = RequestBuilder.Build<GetOrCreateUserRequest>();
            request.UserName = "newuser";
            request.CustomUserData = "blah";
            request.PasswordHash = User.GeneratePasswordDigest("newuser", Config.Current.AuthenticationRealm, "password2");
            response = Service.GetOrCreateUser(request);
            Assert.AreEqual(0, response.Errors.Count);

        }

        [Test]
        public void TestEditAndGetUserGames()
        {
            User user = new User
                            {
                                Id = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F"),
                                Name = "jim",
                                PasswordHash = "password".ComputeMD5()
                            };
            GameSourceRepository.Current.Insert(user);
            GameSourceRepository.Current.SubmitChanges();

            //try to give access to non-existant user. (will fail)
            var request = RequestBuilder.Build<EditUserGamesRequest>();
            request.AllowAccess = true;
            request.GameUid = "game1";
            request.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A5678");
            var response = Service.EditUserGames(request);
            Assert.AreEqual(1, response.Errors.Count);

            //try to give access to some other developers game (will fail)
            request = RequestBuilder.Build<EditUserGamesRequest>();
            request.AllowAccess = true;
            request.GameUid = "game2";
            request.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            response = Service.EditUserGames(request);
            Assert.AreEqual(1, response.Errors.Count);

            //now the happy day case.
            request = RequestBuilder.Build<EditUserGamesRequest>();
            request.AllowAccess = true;
            request.GameUid = "game1";
            request.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            response = Service.EditUserGames(request);

            Assert.AreEqual(0,response.Errors.Count);
            UserGame userGame = GameSourceRepository.Current.Get<UserGame>().SingleOrDefault(ug => ug.UserId == user.Id);
            Assert.IsNotNull(userGame);

            //check that the get user games method picks up the new entry
            var getRequest = RequestBuilder.Build<GetUserGamesRequest>();
            getRequest.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            var getResponse = Service.GetUserGames(getRequest);
            Assert.AreEqual(0, getResponse.Errors.Count);
            Assert.AreEqual(1, getResponse.UserGames.Count);
            Assert.AreEqual("game1", getResponse.UserGames[0].Uid);
            Assert.AreEqual("http://games.junkship.org/games/k-4eNMVB8Eik19TyNLnRBQ.mza?hmac=" + Cryptography.GenerateHMac(_developer1.SecretKey, new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D105").ToString().ToLowerInvariant(), "2009-10-10T00:00:00Z") + "&timestamp=2009-10-10T00:00:00Z", getResponse.UserGames[0].LatestVersion.AuthenticatedDownloadURL);
        
            //now revoke access to our game
            request = RequestBuilder.Build<EditUserGamesRequest>();
            request.AllowAccess = false;
            request.GameUid = "game1";
            request.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            response = Service.EditUserGames(request);
            Assert.AreEqual(0, response.Errors.Count);

            userGame = GameSourceRepository.Current.Get<UserGame>().SingleOrDefault(ug => ug.UserId == user.Id);
            Assert.IsNull(userGame);

            //check that the get user games method no longer lists the revoked game
            getRequest = RequestBuilder.Build<GetUserGamesRequest>();
            getRequest.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            getResponse = Service.GetUserGames(getRequest);
            Assert.AreEqual(0, getResponse.Errors.Count);
            Assert.AreEqual(0, getResponse.UserGames.Count);

            //lets say another developer gives this user access. lets check that thier game doesn't show up in this developers list.
            UserGame otherUserGame = new UserGame
                                         {
                                             Id = Guid.NewGuid(),
                                             UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F"),
                                             GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B")//game2
                                         };
            GameSourceRepository.Current.Insert(otherUserGame);
            GameSourceRepository.Current.SubmitChanges();

            getRequest = RequestBuilder.Build<GetUserGamesRequest>();
            getRequest.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            getResponse = Service.GetUserGames(getRequest);
            Assert.AreEqual(0, getResponse.Errors.Count);
            Assert.AreEqual(0, getResponse.UserGames.Count);

            //make sure we can't revoke access to some other developers game.
            request = RequestBuilder.Build<EditUserGamesRequest>();
            request.AllowAccess = false;
            request.GameUid = "game2";
            request.UserId = new Guid("DB6798D9-AB1F-482E-BD46-D288B05A333F");
            response = Service.EditUserGames(request);
            Assert.AreEqual(1, response.Errors.Count);
        }


        [Test]
        public void TestDeleteGameIncorrectDeveloper()
        {
            DefaultFileServerGameData gameDataEntity = new DefaultFileServerGameData
                                                           {
                                                               GameFile = "/games/dev/game1.mza",
                                                               Id = new Guid("6E559F1B-4DF2-1B25-09F4-21548F4771D7")
                                                           };
            GameSourceRepository.Current.Insert(gameDataEntity);

            GameVersion gameVersion = new GameVersion
                                          {
                                              CreatedDate = TimeService.Current.Now,
                                              Description = "game2 version 1",
                                              GameId = new Guid("6474EF7B-9B04-4635-949B-5B7216D1395B"),
                                              Id = new Guid("341EEE93-41C5-48F0-A4D7-D4F234B9D112"),
                                              IsUpdate = false,
                                              Published = false,
                                              Version = "1.5.0.0",
                                              GameDataId = gameDataEntity.Id.ToString(),
                                              Md5Hash = "md5"
                                          };
            GameSourceRepository.Current.Insert(gameVersion);
            GameSourceRepository.Current.SubmitChanges();

            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games").AddDirectory("dev").AddFile("gameversion1.mza", "file content");

            var request = RequestBuilder.Build<DeleteGameRequest>();
            request.GameUid = "game2";
            var response = Service.DeleteGame(request);
            Assert.AreEqual(1, response.Errors.Count);
            Assert.AreEqual("AccessDenied", response.Errors.First().Code);
        }
    }
}