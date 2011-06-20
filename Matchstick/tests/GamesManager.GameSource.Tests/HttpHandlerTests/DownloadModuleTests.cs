using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Handlers;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Tests.Mocks;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;


namespace MGDF.GamesManager.GameSource.Tests.HttpHandlerTests
{
    [TestFixture]
    public class DownloadHandlerTests: HttpHandlerTestFixtureBase<DownloadModule>
    {
        private Developer _dev1, _dev2;

        protected override void InsertTestData()
        {
            MockFileSystem filesystem = (MockFileSystem) FileSystem.Current;
            MockDirectory directory = filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("games");
            var game1Data = GenerateDataBlock(16383);
            string game1Hash = GenerateMd5Hash(game1Data);
            directory.AddFile("gameversion1.mza", game1Data);
            var game2Data = GenerateDataBlock(16385);
            string game2Hash = GenerateMd5Hash(game2Data);
            directory.AddFile("gameversion2.mza", game2Data);

            _dev1 = new Developer()
                        {
                            Id = Guid.NewGuid(),
                            Name = "dev1",
                            Uid = "dev1",
                        };
            _dev1.GenerateDeveloperKeySecretKeyPair();

            _dev2 = new Developer()
                        {
                            Id = Guid.NewGuid(),
                            Name = "dev2",
                            Uid = "dev2",
                        };
            _dev2.GenerateDeveloperKeySecretKeyPair();

            Game game1 = new Game
                             {
                                 Id = new Guid("2DF7A112-5212-445D-8954-59A4918C6A30"),
                                 Description = "game1 desc",
                                 Homepage = "http://www.example.com",
                                 InterfaceVersion = 1,
                                 Name = "game1",
                                 Uid = "game1",
                                 RequiresAuthentication = true,
                                 DeveloperId = _dev1.Id
                             };

            Game game2 = new Game
                             {
                                 Id = new Guid("2DF7A112-5212-445D-8954-59A4918C6A31"),
                                 Description = "game2 desc",
                                 Homepage = "http://www.example.com",
                                 InterfaceVersion = 1,
                                 Name = "game2",
                                 Uid = "game2",
                                 RequiresAuthentication = false,
                                 DeveloperId = _dev2.Id
                             };

            DefaultFileServerGameData game1DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "/games/gameversion1.mza",
                                                                Id = new Guid("6E679F1B-4DF2-4B25-95F4-21548F4771D7")
                                                            };

            GameVersion gameVersion1 = new GameVersion
                                           {
                                               CreatedDate = TimeService.Current.Now,
                                               Description = "gameversion 1 desc",
                                               GameId = new Guid("2DF7A112-5212-445D-8954-59A4918C6A30"),
                                               Id = new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C"),
                                               Published = true,
                                               GameDataId = game1DataEntity.Id.ToString(),
                                               IsUpdate = false,
                                               Version = "1.0.0.1",
                                               Md5Hash = game1Hash
                                           };

            DefaultFileServerGameData game2DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "/games/gameversion2.mza",
                                                                Id = new Guid("6E679F1B-4DF2-4B25-85F4-21548F4771D7")
                                                            };

            GameVersion gameVersion2 = new GameVersion
                                           {
                                               CreatedDate = TimeService.Current.Now,
                                               Description = "gameversion 2 desc",
                                               GameId = new Guid("2DF7A112-5212-445D-8954-59A4918C6A31"),
                                               Id = new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9F"),
                                               Published = true,
                                               GameDataId = game2DataEntity.Id.ToString(),
                                               IsUpdate = false,
                                               Version = "1.1.0.1",
                                               Md5Hash = game2Hash
                                           };

            DefaultFileServerGameData game3DataEntity = new DefaultFileServerGameData
                                                            {
                                                                GameFile = "/games/gameversion2.mza",
                                                                Id = new Guid("6E679F1B-4DF2-4B25-85F4-11548F4771D7")
                                                            };

            GameVersion gameVersion3 = new GameVersion
                                           {
                                               CreatedDate = TimeService.Current.Now,
                                               Description = "gameversion 3 desc",
                                               GameId = new Guid("2DF7A112-5212-445D-8954-59A4918C6A31"),
                                               Id = new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9E"),
                                               Published = false,
                                               GameDataId = game3DataEntity.Id.ToString(),
                                               IsUpdate = false,
                                               Version = "1.1.0.2",
                                               Md5Hash = game2Hash
                                           };

            User user = User.Create("jimbob", Config.Current.AuthenticationRealm, "password1");
            User user1 = User.Create("noaccess", Config.Current.AuthenticationRealm, "password1");

            UserGame userGame = new UserGame
                                    {
                                        Id = new Guid("C94CA419-E2B3-4079-8591-4FB1596AF455"),
                                        GameId = game1.Id,
                                        UserId = user.Id
                                    };

            GameSourceRepository.Current.Insert(_dev1);
            GameSourceRepository.Current.Insert(_dev2);

            GameSourceRepository.Current.Insert(user);
            GameSourceRepository.Current.Insert(user1);

            GameSourceRepository.Current.Insert(game1);
            GameSourceRepository.Current.Insert(game2);

            GameSourceRepository.Current.Insert(userGame);

            GameSourceRepository.Current.Insert(game1DataEntity);
            GameSourceRepository.Current.Insert(game2DataEntity);
            GameSourceRepository.Current.Insert(game3DataEntity);

            GameSourceRepository.Current.Insert(gameVersion1);
            GameSourceRepository.Current.Insert(gameVersion2);
            GameSourceRepository.Current.Insert(gameVersion3);
            GameSourceRepository.Current.SubmitChanges();
        }

        [Test]
        public void TestDownloadUnauthorizedUnpublishedOrNonExistantGames()
        {
            //game2/1.1.0.2.mza unpublished
            DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames("/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9E").Encode()+".mza", 404);
            //non existant
            DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames("/games/"+new Guid("1A58F0FF-98A2-4381-BA6F-EEEFBEFA6C9F").Encode()+".mza", 404);
            DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames("/games/omgponies.mza", 404);
            DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames("/game4/1.1.0.2.mza", 404);
            //game1/1.0.0.1 published but access restricted
            DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames("/games/"+new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode()+".mza", 401);
        }

        public void DoTestDownloadUnauthorizedUnpublishedOrNonExistantGames(string url, int responseCode)
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com"+url),
                                  Path = url
                              };

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(responseCode, response.StatusCode);
        }


        [Test]
        public void TestDownloadUnauthenticatedGame()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/games/"+new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9F").Encode()+".mza"),
                                  Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9F").Encode() + ".mza"
                              };

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.IsFalse(response.ResponseEnded);
            Assert.AreEqual("application/x-mgdf-archive",response.ContentType);
            Assert.AreEqual("attachment; filename=\"game2.mza\"", response.Headers[Headers.ContentDisposition]);
        }

        [Test]
        public void TestDownloadLatestGameVersion()
        {
            var request = new MockHttpRequest
            {
                HttpMethod = "GET",
                Url = new Uri("http://www.example.com/games/game1/latestVersion"),
                Path = "/games/game1/latestVersion"
            };

            var response = new MockHttpResponse
            {
                IsClientConnected = true
            };

            Handler.ProcessRequest(request, response);

            Assert.IsTrue(response.ResponseEnded);
            Assert.AreEqual(302,response.StatusCode);
            Assert.AreEqual("http://games.junkship.org/Downloads/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza", response.Headers[Headers.Location]);
        }

        [Test]
        public void TestDownloadAuthenticatedGame()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"),
                                  Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                              };

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);
            Assert.AreEqual(@"Digest realm=""example.com"", algorithm=MD5, qop=""auth"",nonce=""323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"", opaque=""fff0581aa2988143ba6f7d1cbefa6c9c""", response.Headers[Headers.WWWAuthenticate]);

            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"),
                              Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                          };

            string ha1 = ("jimbob:example.com:password1").ComputeMD5();
            string ha2 = ("GET:/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza").ComputeMD5();
            string responseHash = (ha1 + ":323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535:00000001:0a4f113b:auth:" + ha2).ComputeMD5();

            request.Headers.Add("Authorization", @"Digest username=""jimbob"",realm=""example.com"",nonce=""323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"",uri=""/games/"+new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode()+@".mza"",qop=auth,nc=00000001,cnonce=""0a4f113b"",response=""" + responseHash + @""",opaque=""fff0581aa2988143ba6f7d1cbefa6c9c""");

            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            Handler.ProcessRequest(request, response);
            Assert.IsFalse(response.ResponseEnded);
            Assert.AreEqual("application/x-mgdf-archive", response.ContentType);
            Assert.AreEqual("attachment; filename=\"game1.mza\"", response.Headers[Headers.ContentDisposition]);

            ((MockTimeService)TimeService.Current).Now = TimeService.Current.Now.AddMinutes(2);//lets make the server nonce stale

            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };
            Handler.ProcessRequest(request, response);//replay the old request

            //auth should pass but we send back a 401 saying that the nonce is stale.
            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);
            Assert.AreEqual(@"Digest realm=""example.com"", algorithm=MD5, qop=""auth"",nonce=""323030392d31302d31305430303a30323a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"", opaque=""fff0581aa2988143ba6f7d1cbefa6c9c"", stale=true", response.Headers[Headers.WWWAuthenticate]);

        }

        [Test]
        public void TestDownloadPreAuthenticatedGame()
        {
            string timestamp = Cryptography.GenerateTimestamp(TimeService.Current.Now);
            string hmac = Cryptography.GenerateHMac(_dev1.SecretKey, "1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C".ToLowerInvariant(), timestamp);
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza?hmac=" + hmac + "&timestamp=" + timestamp),
                                  Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                              };

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.IsFalse(response.ResponseEnded);
            Assert.AreEqual("application/x-mgdf-archive", response.ContentType);
            Assert.AreEqual("attachment; filename=\"game1.mza\"", response.Headers[Headers.ContentDisposition]);

            //test that users can't use hmacs generated for different game versions to download this game version.
            hmac = Cryptography.GenerateHMac(_dev1.SecretKey, "FFE8F0FF-98A2-4381-BA6F-7D1CBEFA6C9C".ToLowerInvariant(), timestamp);
            request = new MockHttpRequest
            {
                HttpMethod = "GET",
                Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza?hmac=" + hmac + "&timestamp=" + timestamp),
                Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
            };

            response = new MockHttpResponse
            {
                IsClientConnected = true
            };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);

            //now check that another dev can't use thier key to create pre authenticated links for another devs games.
            hmac = Cryptography.GenerateHMac(_dev2.SecretKey, "1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C".ToLowerInvariant(), timestamp);
            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza?hmac=" + hmac + "&timestamp=" + timestamp),
                              Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                          };

            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);

            //now check that the pre authenticated links expire after 15 mins.
            hmac = Cryptography.GenerateHMac(_dev1.SecretKey, "1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C".ToLowerInvariant(), timestamp);
            ((MockTimeService) TimeService.Current).Now = TimeService.Current.Now.AddMinutes(16);

            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza?hmac=" + hmac + "&timestamp=" + timestamp),
                              Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                          };

            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);
        }

        [TestCase("noaccess","password1")]//correct details but this user doesn't have access to this game
        [TestCase("jimbob", "password2")]//wrong password
        [TestCase("jimbob1", "password2")]//no such user
        public void TestDownloadAuthenticatedGameIncorrectDetails(string username,string password)
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"),
                                  Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                              };

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);
            Assert.AreEqual(@"Digest realm=""example.com"", algorithm=MD5, qop=""auth"",nonce=""323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"", opaque=""fff0581aa2988143ba6f7d1cbefa6c9c""", response.Headers[Headers.WWWAuthenticate]);

            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"),
                              Path = "/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza"
                          };

            string ha1 = (username + ":example.com:" + password).ComputeMD5();
            string ha2 = ("GET:/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + ".mza").ComputeMD5();
            string responseHash = (ha1 + ":323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535:00000001:0a4f113b:auth:" + ha2).ComputeMD5();

            request.Headers.Add("Authorization", @"Digest username=""" + username + @""",realm=""example.com"",nonce=""323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"",uri=""/games/" + new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C").Encode() + @".mza"",qop=auth,nc=00000001,cnonce=""0a4f113b"",response=""" + responseHash + @""",opaque=""fff0581aa2988143ba6f7d1cbefa6c9c""");

            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            Handler.ProcessRequest(request, response);

            //access denied
            Assert.AreEqual(401, response.StatusCode);
            Assert.IsTrue(response.ResponseEnded);
            Assert.AreEqual(@"Digest realm=""example.com"", algorithm=MD5, qop=""auth"",nonce=""323030392d31302d31305430303a30303a30305a7c3132372e302e302e317c6334393239353039303632343835636332653933663161653236376638353535"", opaque=""fff0581aa2988143ba6f7d1cbefa6c9c""", response.Headers[Headers.WWWAuthenticate]);
        }
    }
}