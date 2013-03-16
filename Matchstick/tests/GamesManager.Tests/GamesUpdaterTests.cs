using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class GamesUpdaterTests : BaseTest
    {
        [SetUp]
        public override void Setup()
        {
            ArchiveFactory.Current = new MockArchiveFactory();
            SettingsManager.Dispose();
            Resources.UninitUserDirectory();
            base.Setup();
        }

        [TearDown]
        public override void TearDown()
        {
            base.TearDown();
        }

        [Test]
        public void TestCheckForUpdateAndDownloadUpdateNoAuth()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.junkship.net/downloads/console.zip", newGameData);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
}
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);

            Assert.IsFalse(FileSystem.Current.GetFile("c:\\temp.zip").Exists);

            //now download the actual update.
            GameDownloader downloader = new GameDownloader(game,"http://www.junkship.net/downloads/console.zip", "c:\\temp.zip", gameMd5, null);
            downloader.Start();

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\temp.zip").Exists);
            Assert.AreEqual(65536, FileSystem.Current.GetFile("c:\\temp.zip").Length);
        }

        [Test]
        public void TestCheckForUpdateAndDownloadUpdateWithAuthCachedCredentials()
        {
            Resources.InitUserDirectory("Console", false);

            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            SettingsManager.Instance.Settings = new GameSettings();
            SettingsManager.Instance.Settings.GameUid = "Console";
            SettingsManager.Instance.Settings.UserName = "user";
            SettingsManager.Instance.Settings.Password = "password1";
            SettingsManager.Instance.Save();

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://www.junkship.net/downloads/console.zip", "user", "password1");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.junkship.net/downloads/console.zip", newGameData);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
}
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);

            Assert.IsFalse(FileSystem.Current.GetFile("c:\\temp.zip").Exists);

            //now download the actual update.
            GameDownloader downloader = new GameDownloader(game, "http://www.junkship.net/downloads/console.zip", "c:\\temp.zip", gameMd5, args=>
                                           {
                                               Assert.Fail("With correct cached credentials the credentials callback shouldn't be invoked.");
                                               return false;
                                           });
            downloader.Start();

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\temp.zip").Exists);
            Assert.AreEqual(65536, FileSystem.Current.GetFile("c:\\temp.zip").Length);
        }

        [Test]
        public void TestCheckForUpdateAndDownloadUpdateWithAuthNoCachedCredentials()
        {
            Resources.InitUserDirectory("Console", false);

            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://www.junkship.net/downloads/console.zip", "user", "password1");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.junkship.net/downloads/console.zip", newGameData);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
}
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);

            Assert.IsFalse(FileSystem.Current.GetFile("c:\\temp.zip").Exists);

            //now download the actual update.
            GameDownloader downloader = new GameDownloader(game, "http://www.junkship.net/downloads/console.zip", "c:\\temp.zip", gameMd5, args =>
            {
                args.UserName = "user";
                args.Password = "password1";
                return true;
            });
            downloader.Start();


            Assert.AreEqual(SettingsManager.Instance.Settings.GameUid,"Console");
            Assert.AreEqual(SettingsManager.Instance.Settings.UserName,"user");
            Assert.AreEqual(SettingsManager.Instance.Settings.Password,"password1");

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\temp.zip").Exists);
            Assert.AreEqual(65536, FileSystem.Current.GetFile("c:\\temp.zip").Length);
        }

        [Test]
        public void TestCheckForUpdateAndDownloadUpdateWithAuthInvalidCachedCredentials()
        {
            Resources.InitUserDirectory("Console", false);

            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            SettingsManager.Instance.Settings = new GameSettings();
            SettingsManager.Instance.Settings.GameUid = "Console";
            SettingsManager.Instance.Settings.UserName = "user";
            SettingsManager.Instance.Settings.Password = "password2";
            SettingsManager.Instance.Save();

            ((MockHttpRequestManager)HttpRequestManager.Current).SetCredentials("http://www.junkship.net/downloads/console.zip", "user", "password1");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.junkship.net/downloads/console.zip", newGameData);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
}
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);

            Assert.IsFalse(FileSystem.Current.GetFile("c:\\temp.zip").Exists);

            //now download the actual update.
            GameDownloader downloader = new GameDownloader(game, "http://www.junkship.net/downloads/console.zip", "c:\\temp.zip", gameMd5, args =>
            {
                args.UserName = "user";
                args.Password = "password1";
                return true;
            });
            downloader.Start();


            Assert.AreEqual(SettingsManager.Instance.Settings.GameUid, "Console");
            Assert.AreEqual(SettingsManager.Instance.Settings.UserName, "user");
            Assert.AreEqual(SettingsManager.Instance.Settings.Password, "password1");

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\temp.zip").Exists);
            Assert.AreEqual(65536, FileSystem.Current.GetFile("c:\\temp.zip").Length);
        }

        [Test]
        public void TestCheckForUpdatePartialUpdatesAvailable()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
},
""UpdateOlderVersions"":[
    {
        ""Url"":""http://www.junkship.net/downloads/console-update.zip"",
        ""MD5"":""" + gameMd5 + @""",
        ""FromVersion"":""0.1""
    },
    {
        ""Url"":""http://www.junkship.net/downloads/console-update1.zip"",
        ""MD5"":""" + gameMd5 + @""",
        ""FromVersion"":""0.1.1.1""
    }
]
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console-update.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);
        }

        [Test]
        public void TestCheckForUpdateNoNewerVersionsAvailable()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx/downloads/1/Console/latest.json", @"{ 
""Latest"":{
""Version"":""0.1"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
},
""UpdateOlderVersions"":[
    {
        ""Url"":""http://www.junkship.net/downloads/console-update.zip"",
        ""MD5"":""" + gameMd5 + @""",
        ""FromVersion"":""0.1""
    },
]
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNull(update.Game);
        }

        [Test]
        public void TestCheckForUpdateInapplicablePartialUpdatesAvailable()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

            var newGameData = GenerateDataBlock(65536);
            var gameMd5 = GenerateMd5Hash(newGameData);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://games.junkship.org/gamesource.asmx", @"{ 
""Latest"":{
""Version"":""1.1.2.4"",
""Url"":""http://www.junkship.net/downloads/console.zip"",
""MD5"":""" + gameMd5 + @"""
},
""UpdateOlderVersions"":[
    {
        ""Url"":""http://www.junkship.net/downloads/console-update1.zip"",
        ""MD5"":""" + gameMd5 + @""",
        ""FromVersion"":""0.1.1.1""
    }
]
}");

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNotNull(update.Game);
            Assert.AreEqual("http://www.junkship.net/downloads/console.zip", update.Game.Url);
            Assert.AreEqual(gameMd5, update.Game.MD5);
            Assert.AreEqual("1.1.2.4", update.Game.Version);
        }

        [Test]
        public void TestCheckForUpdateNoUpdatesAvailable()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));


            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            var update = UpdateChecker.CheckForUpdate(game);

            Assert.IsNotNull(update);
            Assert.IsNull(update.Game);
        }


        [TestCase(false)]
        [TestCase(true)]
        public void TestUpdateLocalGameWithFullInstaller(bool includeGdfDll)
        {
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.zip");
            new MockArchiveFile(archive, "game.json", ReadTextFile("console.json"));
            new MockArchiveFile(archive, "preferences.json", ReadTextFile("preferences.json"));
            if (includeGdfDll)
            {
                new MockArchiveFile(archive, "gdf.dll", "GAMES_EXPLORER_DEFINITION");
            }
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.zip", archive);

            var gameInstall = new GameInstall("C:\\Documents and Settings\\user\\desktop\\game.zip");
            Assert.IsTrue(gameInstall.IsValid);

            GameUpdater updater = new GameUpdater(gameInstall);
            updater.Start();

            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\preferences.json").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game\\content").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game\\bin").Exists);

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            Assert.IsTrue(game.IsValid);

            var key = Registry.Current.CreateSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");

            GameRegistrar registrar = new GameRegistrar(true, game);
            registrar.Start();

            //assert the shortcuts are in the right place
            key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\resources\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\game", key.GetValue("InstallLocation"));
            Assert.AreEqual("0.1", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);

            //assert the the game has been added to the games explorer
            Assert.AreEqual(includeGdfDll, GameExplorer.Current.IsInstalled("c:\\program files\\MGDF\\game\\gdf.dll"));

            //deregister the game
            registrar = new GameRegistrar(false, game);
            registrar.Start();

            //assert the shortcut has been removed
            key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNull(key);

            //shortcuts should have been removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);

            if (includeGdfDll)
            {
                //assert the the game has been removed from the games explorer
                Assert.IsTrue(!GameExplorer.Current.IsInstalled("c:\\program files\\MGDF\\game\\gdf.dll"));
            }
        }

        [Test]
        public void TestInstallLocalUpdateToInstalledGame()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game\\content").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\content\\test.json").WriteText("blah");
            Registry.Current.CreateSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");

            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.zip");
            new MockArchiveFile(archive, "game.json", @"{
  ""gameuid"":""Console"",
  ""gamename"":""Lua Console"",
  ""description"":""A Lua command console for interacting with the MGDF system"",
  ""version"":""1.0.0.0"",
  ""interfaceversion"":""1"",
  ""developeruid"":""no-8"",
  ""developername"":""no8 interactive"",
  ""homepage"":""http://www.junkship.org"",
  ""gamesourceservice"":""http://games.junkship.org/gamesource.asmx"",
  ""statisticsservice"":""http://statistics.junkship.org/statisticsservice.asmx"",
  ""statisticsprivacypolicy"":""We wont use ur informationz"",
  ""supportemail"":""support@junkship.org""
}");
            new MockArchiveFile(archive, "update.json", ReadTextFile("update.json"));
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.zip", archive);

            GameInstall install = new GameInstall("C:\\Documents and Settings\\user\\desktop\\game.zip");
            Assert.IsTrue(install.IsValid);

            GameUpdater updater = new GameUpdater(install);
            updater.Start();

            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game\\content").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game\\bin").Exists);
            //did the update remove the file specified
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\content\\test.json").Exists);

            Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
            Assert.IsTrue(game.IsValid);

            GameRegistrar registrar = new GameRegistrar(true, game);
            registrar.Start();

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\resources\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\game", key.GetValue("InstallLocation"));
            Assert.AreEqual("1.0.0.0", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);

            registrar = new GameRegistrar(false, game);
            registrar.Start();

            //assert the registry key has been removed
            key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNull(key);

            //shortcuts should have been removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }
    }
}
