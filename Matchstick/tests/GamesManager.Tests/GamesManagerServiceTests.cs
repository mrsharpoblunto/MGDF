using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class GamesManagerServiceTests: BaseTest
    {
        [SetUp]
        public override void Setup()
        {
            ArchiveFactory.Current = new MockArchiveFactory();
            base.Setup();
        }

        [TearDown]
        public override void TearDown()
        {
            GamesManagerService.Instance.Dispose();
            base.TearDown();
        }

        [TestCase(false)]
        [TestCase(true)]
        public void TestInstallAndUninstallLocalGame(bool includeGdfDll)
        {
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", ReadTextFile("console.xml"));
            new MockArchiveFile(archive, "preferences.xml", ReadTextFile("preferences.xml"));
            if (includeGdfDll)
            {
                new MockArchiveFile(archive, "gdf.dll", "GAMES_EXPLORER_DEFINITION");
            }
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(0, errors.Count);

            while (gameInfo.InstallState!=InstallState.Installed)
            {
                if (gameInfo.InstallState == InstallState.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }
            Assert.AreEqual(InstallState.Installed,gameInfo.InstallState);

            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\preferences.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console\\content").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console\\bin").Exists);

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\games\\core\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\games\\Console", key.GetValue("InstallLocation"));
            Assert.AreEqual("c:\\program files\\MGDF\\GamesManager.exe -uninstall:Console", key.GetValue("UninstallString"));
            Assert.AreEqual("0.1", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
            
            //assert the the game has been added to the games explorer
            Assert.AreEqual(includeGdfDll,GameExplorer.Current.IsInstalled("c:\\program files\\MGDF\\games\\Console\\gdf.dll"));

            errors = new List<string>();
            GamesManagerService.Instance.Uninstall("Console", errors);
            Assert.AreEqual(0, errors.Count);

            while (gameInfo.InstallState != InstallState.NotInstalled)
            {
                if (gameInfo.InstallState == InstallState.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);

            //assert the files have been removed
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);

            //assert the shortcuts have been removed
            key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE/Microsoft/Windows/CurrentVersion/Console");
            Assert.IsNull(key);

            //shortcuts should have been removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);

            if (includeGdfDll)
            {
                //assert the the game has been removed from the games explorer
                Assert.IsTrue(!GameExplorer.Current.IsInstalled("c:\\program files\\MGDF\\games\\Console\\gdf.dll"));
            }
        }

        [Test]
        public void TestInstallInvalidLocalGame()
        {
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", ReadTextFile("console.xml"));
            new MockArchiveFile(archive, "preferences.xml", ReadTextFile("preferences.xml"));
            new MockArchiveFile(archive, "bin");
            //missing content
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("Invalid game installer file", errors[0]);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);
        }

        [Test]
        public void TestLocalInstallAlreadyInstalledGame()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", ReadTextFile("console.xml"));
            new MockArchiveFile(archive, "preferences.xml", ReadTextFile("preferences.xml"));
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("Game already installed", errors[0]);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);
        }

        [Test]
        public void TestInstallLocalUpdateToNonInstalledGame()
        {
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", ReadTextFile("console.xml"));
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.0.0.1</updateminversion>
  <updatemaxversion>0.0.0.2</updatemaxversion>
  <removefiles />
</mgdf:update>");
            new MockArchiveFile(archive, "preferences.xml", ReadTextFile("preferences.xml"));
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            var boot = new MockArchiveFile(archive, "boot");
            new MockArchiveFile(boot, "gameState.xml", ReadTextFile("gameState.xml"));
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("Cannot install an update-only game version", errors[0]);

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);
        }

        [Test]
        public void TestInstallLocalUpdateWithIncorrectVersionToInstalledGame()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0.0.2</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0.0.0</updateminversion>
  <updatemaxversion>1.0.0.1</updatemaxversion>
  <removefiles />
</mgdf:update>");
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("Game version cannot update the installed version", errors[0]);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);
        }

        [Test]
        public void TestUpdateRemoteGame()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));
            
            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>bin\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count>0)
            {
                if (gameInfo.InstallState == InstallState.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);


            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test1.dll").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test.dll").Exists);
            Assert.AreEqual(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").ReadText(), @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer file was removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\games\\core\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\games\\Console", key.GetValue("InstallLocation"));
            Assert.AreEqual("c:\\program files\\MGDF\\GamesManager.exe -uninstall:Console", key.GetValue("UninstallString"));
            Assert.AreEqual("1.1.0.0", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestUpdateRemoteGameConnectionError()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data, (s, e) => { throw new Exception("Connection Closed"); }, null);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>modules\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.InstallState == InstallState.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //check that the old version wasn't ovewritten with a new version.
            string expectedContent = ReadTextFile("console.xml");
            Assert.AreEqual(expectedContent, FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").ReadText());

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer files were removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);
        }

        [Test]
        public void TestUpdateRemoteGamePauseAndResume()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            bool doPause = true;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data,(s, e) =>
                {
                    if (doPause)
                    {
                        GamesManagerService.Instance.PausePendingOperations("Console");
                        doPause = false;
                    }
                }, null);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>bin\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count>0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Paused)
                {
                    break;
                }
                else if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }
            Assert.AreEqual(PendingOperationStatus.Paused, gameInfo.PendingOperations[0].Status);

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Updating, gameInfo.InstallState);

            //check the queue file has been persisted.
            var queueFile = FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml");
            string queueFileContents = queueFile.ReadText();
            string expectedQueueFile =
                @"﻿<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
@"<pendingoperationqueue>" + "\r\n" +
@"	<pendinggamedownload>"+"\r\n"+
@"		<status>Paused</status>"+"\r\n"+
@"		<progress>16384</progress>"+"\r\n"+
@"		<total>35000</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000000.mza</destinationfilename>"+"\r\n"+
@"		<sourceurl>http://repo/downloads/game.mza</sourceurl>"+"\r\n"+
@"		<md5hash>8f5936d96d62f0f093d2b947efcea284</md5hash>"+"\r\n"+
@"		<gameuid>Console</gameuid>"+"\r\n"+
@"	</pendinggamedownload>"+"\r\n"+
@"	<pendinggamedownload>"+"\r\n"+
@"		<status>Queued</status>"+"\r\n"+
@"		<progress>0</progress>"+"\r\n"+
@"		<total>1</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000001.mza</destinationfilename>"+"\r\n"+
@"		<sourceurl>http://repo/downloads/game1.mza</sourceurl>"+"\r\n"+
@"		<md5hash>3b9243337d3aa965fa38ad66bb547c42</md5hash>"+"\r\n"+
@"		<gameuid>Console</gameuid>"+"\r\n"+
@"	</pendinggamedownload>"+"\r\n"+
@"</pendingoperationqueue>";
            Assert.AreEqual(expectedQueueFile, queueFileContents);

            //check the first partial download exists
            Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);


            GamesManagerService.Instance.ResumePendingOperations("Console");

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);


            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test1.dll").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test.dll").Exists);
            Assert.AreEqual(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").ReadText(), @"<?xml version=""1.0"" encoding=""UTF-8""?>"+"\r\n"+
@"<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">"+"\r\n"+
@"  <gameuid>Console</gameuid>"+"\r\n"+
@"  <gamename>Lua Console</gamename>"+"\r\n"+
@"  <description>A Lua command console for interacting with the MGDF system</description>"+"\r\n"+
@"  <version>1.1.0.0</version>"+"\r\n"+
@"  <interfaceversion>1</interfaceversion>"+"\r\n"+
@"  <developeruid>no-8</developeruid>"+"\r\n"+
@"  <developername>no8 interactive</developername>"+"\r\n"+
@"  <homepage>http://www.junkship.org</homepage>"+"\r\n"+
@"  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>"+"\r\n"+
@"  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>"+"\r\n"+
@"  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>"+"\r\n"+
@"  <supportemail>support@junkship.org</supportemail>"+"\r\n"+
@"</mgdf:game>");

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer files were removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\games\\core\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\games\\Console", key.GetValue("InstallLocation"));
            Assert.AreEqual("c:\\program files\\MGDF\\GamesManager.exe -uninstall:Console", key.GetValue("UninstallString"));
            Assert.AreEqual("1.1.0.0", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestUninstallPausedGameUpdate()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            bool doPause = true;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data, (s, e) =>
            {
                if (doPause)
                {
                    GamesManagerService.Instance.PausePendingOperations("Console");
                    doPause = false;
                }
            }, null);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>modules\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Paused)
                {
                    break;
                }
                else if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }
            Assert.AreEqual(PendingOperationStatus.Paused, gameInfo.PendingOperations[0].Status);

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Updating, gameInfo.InstallState);

            //check the queue file has been persisted.
            var queueFile = FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml");
            string queueFileContents = queueFile.ReadText();
            string expectedQueueFile =
                @"﻿<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
"<pendingoperationqueue>"+"\r\n"+
"	<pendinggamedownload>"+"\r\n"+
"		<status>Paused</status>"+"\r\n"+
"		<progress>16384</progress>"+"\r\n"+
"		<total>35000</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000000.mza</destinationfilename>"+"\r\n"+
"		<sourceurl>http://repo/downloads/game.mza</sourceurl>"+"\r\n"+
"		<md5hash>8f5936d96d62f0f093d2b947efcea284</md5hash>"+"\r\n"+
"		<gameuid>Console</gameuid>"+"\r\n"+
"	</pendinggamedownload>"+"\r\n"+
"	<pendinggamedownload>"+"\r\n"+
"		<status>Queued</status>"+"\r\n"+
"		<progress>0</progress>"+"\r\n"+
"		<total>1</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000001.mza</destinationfilename>"+"\r\n"+
"		<sourceurl>http://repo/downloads/game1.mza</sourceurl>"+"\r\n"+
"		<md5hash>3b9243337d3aa965fa38ad66bb547c42</md5hash>"+"\r\n"+
"		<gameuid>Console</gameuid>"+"\r\n"+
"	</pendinggamedownload>"+"\r\n"+
"</pendingoperationqueue>";
            Assert.AreEqual(expectedQueueFile, queueFileContents);

            //check the first partial download exists
            Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);


            List<string> uninstallErrors = new List<string>();
            GamesManagerService.Instance.Uninstall("Console", uninstallErrors);
            Assert.AreEqual(0, uninstallErrors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game has been uninstalled
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);


            //check that the game cancelled the update and uninstalled instead.
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);

            //check that all the games content was removed
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer files were removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);

            //assert the shortcuts are removed
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNull(key);

            //assert the shortcuts are removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestUninstallGameWhileRemoteGameUpdateInProgress()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            bool doUninstall = true;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data, (s, e) =>
            {
                if (doUninstall)
                {
                    List<string> uninstallErrors = new List<string>();
                    GamesManagerService.Instance.Uninstall("Console", uninstallErrors);
                    Assert.AreEqual(0, uninstallErrors.Count);
                    doUninstall = false;
                }
            }, null);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>modules\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the game cancelled the update and uninstalled instead.
            Assert.AreEqual(InstallState.NotInstalled, gameInfo.InstallState);

            //check that all the games content was removed
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer files were removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);

            //assert the shortcuts are removed
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNull(key);

            //assert the shortcuts are removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestUpdateRemoteGameShutdownAndRestart()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //set up the mock remote data to be downloaded
            HttpRequestManager.Current = new MockHttpRequestManager();
            byte[] update1Data = GenerateDataBlock(35000);
            byte[] update2Data = GenerateDataBlock(39000);
            bool doShutdown = false;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.mza", update1Data, (s, e) =>
            {
                if (!doShutdown)
                {
                    GamesManagerService.Instance.PausePendingOperations("Console");
                    doShutdown = true;
                }
            }, null);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game1.mza", update2Data);
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://repo/downloads/game.png", GenerateDataBlock(1624));

            //create the fake archive contents to install once the download is complete.
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>0.1</updateminversion>
  <updatemaxversion>0.9</updatemaxversion>
  <removefiles />
</mgdf:update>");
            var modules = new MockArchiveFile(archive, "bin");
            new MockArchiveFile(modules, "test.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza", archive);


            MockArchiveFile archive2 = new MockArchiveFile(null, "C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza");
            new MockArchiveFile(archive2, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.1.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive2, "update.xml", @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:update xmlns:mgdf=""http://schemas.matchstickframework.org/2007/update"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <updatedescription>An update</updatedescription>
  <updateminversion>1.0</updateminversion>
  <updatemaxversion>1.0</updatemaxversion>
  <removefiles>
    <file>bin\test.dll</file>
  </removefiles>
</mgdf:update>");
            var modules2 = new MockArchiveFile(archive2, "bin");
            new MockArchiveFile(modules2, "test1.dll", "1337_CODEZ");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("c:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza", archive2);

            GameUpdate remoteGame = new GameUpdate
            {
                Uid = "Console",
                Name = "Lua Console",
                InterfaceVersion = 1,
                Description = "blah",
                Developer = new Developer { Uid = "no8" },
                Homepage = "http://www.junkship.org"
            };
            GameVersionUpdate update1 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update1Data),
                Version = "1.0",
                UpdateMinVersion = "0.1",
                UpdateMaxVersion = "0.9",
            };
            GameVersionUpdate update2 = new GameVersionUpdate
            {
                DownloadURL = "http://repo/downloads/game1.mza",
                Game = remoteGame,
                IsUpdate = true,
                Md5Hash = GenerateMd5Hash(update2Data),
                Version = "1.1",
                UpdateMinVersion = "1.0",
                UpdateMaxVersion = "1.0",
            };

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Update("Console", new List<GameVersionUpdate> { update1, update2 }, errors);

            Assert.AreEqual(0, errors.Count);

            //wait until the shutdown has been initiated
            while (!doShutdown)
            {
                Thread.Sleep(0);
            }
            //then simulate a complete shutdown of the service.
            GamesManagerService.Instance.Dispose();

            //check the queue file has been persisted.
            var queueFile = FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml");
            string queueFileContents = queueFile.ReadText();
            string expectedQueueFile =
                @"﻿<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
@"<pendingoperationqueue>"+"\r\n"+
@"	<pendinggamedownload>"+"\r\n"+
@"		<status>Paused</status>"+"\r\n"+
@"		<progress>16384</progress>"+"\r\n"+
@"		<total>35000</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000000.mza</destinationfilename>"+"\r\n"+
@"		<sourceurl>http://repo/downloads/game.mza</sourceurl>"+"\r\n"+
@"		<md5hash>8f5936d96d62f0f093d2b947efcea284</md5hash>"+"\r\n"+
@"		<gameuid>Console</gameuid>"+"\r\n"+
@"	</pendinggamedownload>"+"\r\n"+
@"	<pendinggamedownload>"+"\r\n"+
@"		<status>Queued</status>"+"\r\n"+
@"		<progress>0</progress>"+"\r\n"+
@"		<total>1</total>"+"\r\n"+
@"		<destinationfilename>c:\program files\MGDF\games\Downloads\00000000-0000-0000-0000-000000000001.mza</destinationfilename>"+"\r\n"+
@"		<sourceurl>http://repo/downloads/game1.mza</sourceurl>"+"\r\n"+
@"		<md5hash>3b9243337d3aa965fa38ad66bb547c42</md5hash>"+"\r\n"+
@"		<gameuid>Console</gameuid>"+"\r\n"+
@"	</pendinggamedownload>" + "\r\n" +
@"</pendingoperationqueue>";
            Assert.AreEqual(expectedQueueFile, queueFileContents);

            //check the first partial download exists
            Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(PendingOperationStatus.Paused,gameInfo.PendingOperations[0].Status);
            Assert.AreEqual(16384, gameInfo.PendingOperations[0].Progress);
            Assert.AreEqual(PendingOperationStatus.Queued, gameInfo.PendingOperations[1].Status);
            Assert.AreEqual(2, gameInfo.PendingOperations.Count);

            //resume the paused downloads
            GamesManagerService.Instance.ResumePendingOperations("Console");

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test1.dll").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\bin\\test.dll").Exists);
            Assert.AreEqual(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").ReadText(), @"<?xml version=""1.0"" encoding=""UTF-8""?>"+"\r\n"+
@"<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">"+"\r\n"+
@"  <gameuid>Console</gameuid>"+"\r\n"+
@"  <gamename>Lua Console</gamename>"+"\r\n"+
@"  <description>A Lua command console for interacting with the MGDF system</description>"+"\r\n"+
@"  <version>1.1.0.0</version>"+"\r\n"+
@"  <interfaceversion>1</interfaceversion>"+"\r\n"+
@"  <developeruid>no-8</developeruid>"+"\r\n"+
@"  <developername>no8 interactive</developername>"+"\r\n"+
@"  <homepage>http://www.junkship.org</homepage>"+"\r\n"+
@"  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>"+"\r\n"+
@"  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>"+"\r\n"+
@"  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>"+"\r\n"+
@"  <supportemail>support@junkship.org</supportemail>"+"\r\n"+
@"</mgdf:game>");

            //check the queue file was removed.
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Console\\pendingoperationqueue.xml").Exists);

            //check the installer files were removed
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000000.mza").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\00000000-0000-0000-0000-000000000001.mza").Exists);

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\games\\core\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\games\\Console", key.GetValue("InstallLocation"));
            Assert.AreEqual("c:\\program files\\MGDF\\GamesManager.exe -uninstall:Console", key.GetValue("UninstallString"));
            Assert.AreEqual("1.1.0.0", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestInstallLocalUpdateToInstalledGame()
        {
            //lets pretend that this game is already installed.
            FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Create();
            FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").WriteText(ReadTextFile("console.xml"));

            var gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive, "game.xml", @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0.0.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no-8</developeruid>
  <developername>no8 interactive</developername>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>
  <statisticsprivacypolicy>We wont use ur informationz</statisticsprivacypolicy>
  <supportemail>support@junkship.org</supportemail>
</mgdf:game>");
            new MockArchiveFile(archive, "update.xml", ReadTextFile("update.xml"));
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            List<string> errors = new List<string>();
            GamesManagerService.Instance.Install("C:\\Documents and Settings\\user\\desktop\\game.mza", errors);
            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count>0)
            {
                if (gameInfo.InstallState == InstallState.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                Thread.Sleep(1);
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }

            //check that the installed game appears on the list of installed games.
            Assert.AreEqual(InstallState.Installed, gameInfo.InstallState);

            //check that all the games content was copied across
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Console\\game.xml").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console\\content").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console\\bin").Exists);

            //assert the shortcuts are in the right place
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF1_Console");
            Assert.IsNotNull(key);
            Assert.AreEqual("c:\\program files\\MGDF\\games\\core\\gamesystemicon.ico", key.GetValue("DisplayIcon"));
            Assert.AreEqual("Lua Console", key.GetValue("DisplayName"));
            Assert.AreEqual("http://www.junkship.org", key.GetValue("URLInfoAbout"));
            Assert.AreEqual(1, key.GetDwordValue("NoModify"));
            Assert.AreEqual(1, key.GetDwordValue("NoRepair"));
            Assert.AreEqual("no8 interactive", key.GetValue("Publisher"));
            Assert.AreEqual("c:\\program files\\MGDF\\games\\Console", key.GetValue("InstallLocation"));
            Assert.AreEqual("c:\\program files\\MGDF\\GamesManager.exe -uninstall:Console", key.GetValue("UninstallString"));
            Assert.AreEqual("1.0.0.0", key.GetValue("DisplayVersion"));

            //assert the shortcuts are in the right place
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsTrue(FileSystem.Current.GetDirectory("c:\\Documents and Settings\\user\\start menu\\no8 interactive").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
            errors = new List<string>();
            GamesManagerService.Instance.Uninstall("Console", errors);
            Assert.AreEqual(0, errors.Count);

            gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            while (gameInfo.PendingOperations.Count > 0)
            {
                if (gameInfo.PendingOperations[0].Status == PendingOperationStatus.Error)
                {
                    Assert.Fail("Pending operation failed.");
                    break;
                }
                gameInfo = GamesManagerService.Instance.GetGameInfo("Console");
            }
            Assert.AreEqual(InstallState.NotInstalled,gameInfo.InstallState);

            //assert the files have been removed
            Assert.IsFalse(FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\games\\Console").Exists);

            //assert the shortcuts have been removed
            key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE/Microsoft/Windows/CurrentVersion/Console");
            Assert.IsNull(key);

            //shortcuts should have been removed
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\desktop\\Lua Console.lnk").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\Documents and Settings\\user\\start menu\\no8 interactive\\Lua Console.lnk").Exists);
        }

        [Test]
        public void TestUninstallNonExistantGame()
        {
            List<string> errors = new List<string>();
            GamesManagerService.Instance.Uninstall("Console", errors);
            Assert.AreEqual(1, errors.Count);
            Assert.AreEqual("Game is not installed", errors[0]);
        }
    }
}
