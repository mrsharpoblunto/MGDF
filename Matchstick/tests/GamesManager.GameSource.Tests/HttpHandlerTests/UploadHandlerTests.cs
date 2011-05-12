using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Handlers;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Tests.Mocks;
using MGDF.GamesManager.ServerCommon;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;
using Constants=MGDF.GamesManager.Common.Constants;

namespace MGDF.GamesManager.GameSource.Tests.HttpHandlerTests
{
    [TestFixture]
    public class UploadHandlerTests: HttpHandlerTestFixtureBase<UploadHandler>
    {
        private Developer _dev;
        private Developer _dev1;
        private GameFragment _gameFragment;
        private DefaultFileServerGameData _gameFragmentData;

        private byte[] _gameData;
        protected override void InsertTestData()
        {
            MockFileSystem filesystem = (MockFileSystem)FileSystem.Current;
            filesystem.AddVolumeRoot("C:").AddDirectory("gamesource").AddDirectory("dev");
            _gameData = GenerateRandomDataBlock(16383);
            string game1Hash = GenerateMd5Hash(_gameData);

            _dev = Developer.Create("dev", "dev", "http://www.example.com");
            _dev.GenerateDeveloperKeySecretKeyPair();

            _dev1 = Developer.Create("dev1", "dev1", "http://www.example.com");
            _dev1.GenerateDeveloperKeySecretKeyPair();


            Game game1 = new Game
                             {
                                 Id = new Guid("2DF7A112-5212-445D-8954-59A4918C6A30"),
                                 Description = "game1 desc",
                                 Homepage = "http://www.example.com",
                                 InterfaceVersion = 1,
                                 Name = "game1",
                                 Uid = "game1",
                                 RequiresAuthentication = true,
                                 DeveloperId =  _dev.Id
                             };

            GameVersion gameVersion1 = new GameVersion
                                           {
                                               CreatedDate = TimeService.Current.Now,
                                               Description = "gameversion 1 desc",
                                               GameId = new Guid("2DF7A112-5212-445D-8954-59A4918C6A30"),
                                               Id = new Guid("1A58F0FF-98A2-4381-BA6F-7D1CBEFA6C9C"),
                                               Published = false,
                                               IsUpdate = false,
                                               Version = "1.0.0.1"
                                           };

            _gameFragmentData = new DefaultFileServerGameData
                                    {
                                        GameFile = "/dev/gameversion1.fragment",
                                        Id = new Guid("6E779F1B-4DF2-4B25-05F4-21548F4771D7")
                                    };
            GameSourceRepository.Current.Insert(_gameFragmentData);

            _gameFragment = new GameFragment
                                {
                                    DeveloperId = _dev.Id,
                                    GameDataId = _gameFragmentData.Id.ToString(),
                                    GameVersionId = gameVersion1.Id,
                                    Id = new Guid("6C1074A3-FAF7-4DA2-98AA-AC6B527B721B"),
                                    PublishOnComplete = true,
                                    Md5Hash = game1Hash,
                                };

            GameSourceRepository.Current.Insert(_dev);
            GameSourceRepository.Current.Insert(_dev1);

            GameSourceRepository.Current.Insert(game1);
            GameSourceRepository.Current.Insert(gameVersion1);
            GameSourceRepository.Current.Insert(_gameFragment);

            GameSourceRepository.Current.SubmitChanges();
        }

        [Test]
        public void TestAuthenticationInvalidDeveloperKey()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.Md5, "1234"); 
            request.Headers.Add(Constants.Headers.CNonce, Guid.NewGuid().ToString());
            request.Headers.Add(Constants.Headers.DeveloperKey,_dev1.DeveloperKey+"A");
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request,response);

            Assert.AreEqual(401,response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"AuthorizationFailed\" message=\"No developer found with this Developer Key\" retry=\"False\" />", reader.ReadToEnd());
        }

        [Test]
        public void TestAuthenticationInvalidHMac()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.Md5, "1234");
            request.Headers.Add(Constants.Headers.CNonce, Guid.NewGuid().ToString());
            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev.SecretKey, TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"AuthorizationFailed\" message=\"Invalid HMac/Timestamp\" retry=\"False\" />", reader.ReadToEnd());
        }

        [Test]
        public void TestAuthenticationInvalidTimestamp()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.Md5, "1234");
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now.AddMinutes(16)));
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+"1234",TimeService.Current.Now.AddMinutes(16)));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(401, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"AuthorizationFailed\" message=\"Invalid HMac/Timestamp\" retry=\"False\" />", reader.ReadToEnd());
        }

        [Test]
        public void TestUploadFragmentMissingHeaders()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.Md5, "1234");
            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+"1234", TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(400, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"Expected MGDF headers were not present or are invalid\" retry=\"False\" />", reader.ReadToEnd());
        }

        [Test]
        public void TestUploadNonExistantFragment()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.Md5, "1234");
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+"1234",TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.FragmentIdHeader, Guid.NewGuid().ToString());
            request.Headers.Add(Constants.Headers.IsLastFragment, false.ToString());

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(404, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"No incomplete Game fragment with this id exists\" retry=\"False\" />", reader.ReadToEnd());
        }

        [Test]
        public void TestUploadFragment()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(Constants.Headers.IsLastFragment, false.ToString());
            var input = new byte[10000];
            Array.Copy(_gameData, input, 10000);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+input.ComputeMD5(), TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            var fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(200, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><success />", reader.ReadToEnd());

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsTrue(fragmentFile.Exists);
            Assert.AreEqual(10000, fragmentFile.Length);

            //now upload the final fragment
            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/upload.axd"),
                              Path = "/upload.axd"
                          };

            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(Constants.Headers.IsLastFragment, true.ToString());
            input = new byte[6383];
            Array.Copy(_gameData,10000, input,0, 6383);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+input.ComputeMD5(), TimeService.Current.Now));


            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            GameVersion version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Id == new Guid("1a58f0ff-98a2-4381-ba6f-7d1cbefa6c9c"));
            Assert.IsFalse(version.Published);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(200, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><success gameVersionId=\"1a58f0ff-98a2-4381-ba6f-7d1cbefa6c9c\" />", reader.ReadToEnd());

            //has the gameversion been updated
            version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Id == new Guid("1a58f0ff-98a2-4381-ba6f-7d1cbefa6c9c"));
            Guid versionDataId = new Guid(version.GameDataId);
            var versionData = GameSourceRepository.Current.Get<DefaultFileServerGameData>().Single(d => d.Id == versionDataId);
            
            Assert.IsTrue(version.Published);
            Assert.AreEqual(_gameFragment.Md5Hash,version.Md5Hash);
            Assert.AreEqual("/dev/gameversion1.mza", versionData.GameFile);

            var gameFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(versionData.GameFile));
            Assert.IsTrue(gameFile.Exists);
            Assert.AreEqual(16383,gameFile.Length);

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath("/dev/gameversion1.fragment"));
            Assert.IsFalse(fragmentFile.Exists);
        }

        [Test]
        public void TestUploadFragmentInvalidHash()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.IsLastFragment, false.ToString());
            var input = new byte[10000];
            Array.Copy(_gameData, input, 10000);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+input.ComputeMD5(), TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            var fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(200, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><success />", reader.ReadToEnd());

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsTrue(fragmentFile.Exists);
            Assert.AreEqual(10000, fragmentFile.Length);

            //now upload the final fragment
            request = new MockHttpRequest
                          {
                              HttpMethod = "GET",
                              Url = new Uri("http://www.example.com/upload.axd"),
                              Path = "/upload.axd"
                          };

            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.IsLastFragment, true.ToString());
            input = new byte[6383];
            Array.Copy(_gameData, 0, input, 0, 6383);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+input.ComputeMD5(), TimeService.Current.Now));


            response = new MockHttpResponse
                           {
                               IsClientConnected = true
                           };

            GameVersion version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Id == new Guid("1a58f0ff-98a2-4381-ba6f-7d1cbefa6c9c"));
            Assert.IsFalse(version.Published);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(400, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"Invalid file hash\" retry=\"False\" />", reader.ReadToEnd());

            //has the gameversion not been updated
            version = GameSourceRepository.Current.Get<GameVersion>().Single(gv => gv.Id == new Guid("1a58f0ff-98a2-4381-ba6f-7d1cbefa6c9c"));
            Assert.IsFalse(version.Published);

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsTrue(fragmentFile.Exists);
            Assert.AreEqual(16383, fragmentFile.Length);
        }

        [Test]
        public void TestUploadFragmentInvalidPartHash()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(MGDF.GamesManager.Common.Constants.Headers.IsLastFragment, false.ToString());
            var input = new byte[10000];
            Array.Copy(_gameData, input, 10000);

            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            string cnonce = Guid.NewGuid().ToString();
            request.Headers.Add(Constants.Headers.CNonce, cnonce);
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, cnonce+input.ComputeMD5(), TimeService.Current.Now));
            
            //however the part we will actually upload isn't going to have the expected hash
            input = new byte[9999];
            Array.Copy(_gameData, input, 9999);
            request.Input = input;

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            var fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(400, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"Invalid file part hash\" retry=\"True\" />", reader.ReadToEnd());

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);
        }

        [Test]
        public void TestUploadFragmentPartTooLarge()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(Constants.Headers.IsLastFragment, false.ToString());
            var input = new byte[10000];
            Array.Copy(_gameData, input, 10000);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Md5, input.ComputeMD5());
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, input.ComputeMD5(), TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            var fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);

            ((MockConfig) Config.Current).MaxUploadPartSize = 9999;
            Handler.ProcessRequest(request, response);

            Assert.AreEqual(400, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"Part content length exceeds the maximum size (" + ((MockConfig)Config.Current).MaxUploadPartSize + " bytes)\" retry=\"False\" />", reader.ReadToEnd());

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);
        }

        [Test]
        public void TestUploadFragmentMissingPartContentHash()
        {
            var request = new MockHttpRequest
                              {
                                  HttpMethod = "GET",
                                  Url = new Uri("http://www.example.com/upload.axd"),
                                  Path = "/upload.axd"
                              };

            request.Headers.Add(Constants.Headers.DeveloperKey, _dev1.DeveloperKey);
            request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(TimeService.Current.Now));
            request.Headers.Add(Constants.Headers.FragmentIdHeader, _gameFragment.Id.ToString());
            request.Headers.Add(Constants.Headers.IsLastFragment, false.ToString());
            var input = new byte[10000];
            Array.Copy(_gameData, input, 10000);
            request.Input = input;
            request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(_dev1.SecretKey, input.ComputeMD5(), TimeService.Current.Now));

            var response = new MockHttpResponse
                               {
                                   IsClientConnected = true
                               };

            var fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);

            Handler.ProcessRequest(request, response);

            Assert.AreEqual(400, response.StatusCode);
            response.Output.Seek(0, SeekOrigin.Begin);
            var reader = new StreamReader(response.Output);
            Assert.AreEqual("<?xml version=\"1.0\" ?><error code=\"InvalidArguments\" message=\"Part content MD5 checksum header missing or invalid\" retry=\"False\" />", reader.ReadToEnd());

            fragmentFile = FileSystem.Current.GetFile(ServerContext.Current.MapPath(_gameFragmentData.GameFile));
            Assert.IsFalse(fragmentFile.Exists);
        }
    }
}