using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class GameInstallerTests : BaseTest
    {
        [SetUp]
        public override void Setup()
        {
            ArchiveFactory.Current = new MockArchiveFactory();
            base.Setup();
        }


        [Test]
        public void LoadGameInstaller()
        {
            MockDirectory desktopDir = (MockDirectory)MockFileSystem.GetDirectory("C:\\Documents and Settings\\user\\desktop");
            desktopDir.AddFile("game.zip","data");


            MockArchiveFile archive = new MockArchiveFile(null,"C:\\Documents and Settings\\user\\desktop\\game.mza");
            new MockArchiveFile(archive,"game.json", ReadTextFile("console.json"));
            new MockArchiveFile(archive, "preferences.json", ReadTextFile("preferences.json"));
            new MockArchiveFile(archive, "content");
            new MockArchiveFile(archive, "bin");
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.mza", archive);

            GameInstall install = new GameInstall("C:\\Documents and Settings\\user\\desktop\\game.mza");
            Assert.IsNotNull(install);
            Assert.IsTrue(install.IsValid);

            Assert.AreEqual("A Lua command console for interacting with the MGDF ",install.Game.Description);
            Assert.AreEqual("http://games.junkship.org/gamesource.asmx", install.Game.UpdateService);
            Assert.AreEqual("http://www.junkship.org", install.Game.Homepage);
            Assert.AreEqual(1, install.Game.InterfaceVersion);
            Assert.IsFalse(install.IsUpdate);
            Assert.AreEqual("Lua Console", install.Game.Name);
            Assert.AreEqual("We wont use ur informationz", install.Game.StatisticsPrivacyPolicy);
            Assert.AreEqual("http://statistics.junkship.org/statisticsservice.asmx", install.Game.StatisticsService);
            Assert.AreEqual("support@junkship.org", install.Game.SupportEmail);
            Assert.AreEqual("Console", install.Game.Uid);
            Assert.AreEqual(new Version(0,1), install.Game.Version);

            Assert.IsNull(install.Update);
            Assert.IsFalse(install.IsUpdate);

            Assert.AreEqual("C:\\Documents and Settings\\user\\desktop\\game.mza",install.InstallerFile);
        }


        [Test]
        public void TestLoadInvalidGame()
        {
            MockArchiveFile archive = new MockArchiveFile(null, "C:\\Documents and Settings\\user\\desktop\\game.zip");
            new MockArchiveFile(archive, "game.json", ReadTextFile("console.json"));
            new MockArchiveFile(archive, "preferences.json", ReadTextFile("preferences.json"));
            new MockArchiveFile(archive, "bin");
            //missing content
            ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\Documents and Settings\\user\\desktop\\game.zip", archive);

            GameInstall install = new GameInstall("C:\\Documents and Settings\\user\\desktop\\game.zip");
            Assert.IsFalse(install.IsValid);
        }
    }
}
