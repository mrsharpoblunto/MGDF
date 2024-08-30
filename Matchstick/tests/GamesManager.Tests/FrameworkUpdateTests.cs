using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.Tests
{
  [TestFixture]
  public class FrameworkUpdateTests : BaseTest
  {
    [TearDown]
    public override void TearDown()
    {
      base.TearDown();
    }

    [Test]
    public void TestCheckForUpdateAndDownloadUpdate()
    {
      FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
      FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

      MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
      gamesManagerFile.WriteText("EXECUTABLE");
      gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 0);

      var newFrameworkData = GenerateDataBlock(65536);
      var frameworkMd5 = GenerateMd5Hash(newFrameworkData);

      ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/1/MGDF.zip", newFrameworkData);
      ((MockHttpRequestManager)HttpRequestManager.Current).ExpectJsonResponse("http://games.junkship.org/gamesource.asmx", new GameUpdate
      {
        Framework = new UpdateDownload
        {
          Version = "1.1.2.4",
          Url = "http://www.matchstickframework.org/downloads/1/MGDF.zip",
          Md5 = frameworkMd5
        }
      });

      Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
      AvailableUpdates update = UpdateChecker.CheckForUpdate(game);

      Assert.IsNotNull(update);
      Assert.IsNotNull(update.Framework);
      Assert.AreEqual("http://www.matchstickframework.org/downloads/1/MGDF.zip", update.Framework.Url);
      Assert.AreEqual(frameworkMd5, update.Framework.Md5);
      Assert.AreEqual("1.1.2.4", update.Framework.Version);

      Assert.IsFalse(FileSystem.Current.GetFile("c:\\temp.zip").Exists);

      //now download the actual update.
      FileDownloader downloader = new FileDownloader("http://www.matchstickframework.org/downloads/1/MGDF.zip", "c:\\temp.zip", frameworkMd5, null);
      downloader.Start();

      Assert.IsTrue(FileSystem.Current.GetFile("c:\\temp.zip").Exists);
      Assert.AreEqual(65536, FileSystem.Current.GetFile("c:\\temp.zip").Length);
    }

    [Test]
    public void TestCheckForUpdateNoUpdateAvailable()
    {
      FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
      FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

      MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
      gamesManagerFile.WriteText("EXECUTABLE");
      gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 0);

      Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
      AvailableUpdates update = UpdateChecker.CheckForUpdate(game);

      Assert.IsNotNull(update);
      Assert.IsNull(update.Framework);
    }

    [Test]
    public void TestCheckForUpdateEqualToCurrentVersionAvailable()
    {
      FileSystem.Current.GetDirectory("c:\\program files\\MGDF\\game").Create();
      FileSystem.Current.GetFile("c:\\program files\\MGDF\\game\\game.json").WriteText(ReadTextFile("console.json"));

      MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
      gamesManagerFile.WriteText("EXECUTABLE");
      gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 0);

      var newFrameworkData = GenerateDataBlock(65536);
      var frameworkMd5 = GenerateMd5Hash(newFrameworkData);

      ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/1/MGDF.zip", newFrameworkData);
      ((MockHttpRequestManager)HttpRequestManager.Current).ExpectJsonResponse("http://games.junkship.org/gamesource.asmx/downloads/1/Console/latest.json", new GameUpdate
      {
        Framework = new UpdateDownload
        {
          Version = "1.0.0.0",
          Url = "http://www.matchstickframework.org/downloads/1/MGDF.zip",
          Md5 = frameworkMd5
        }
      });

      Game game = new Game("c:\\program files\\MGDF\\game\\game.json");
      AvailableUpdates update = UpdateChecker.CheckForUpdate(game);

      Assert.IsNotNull(update);
      Assert.IsNull(update.Framework);
    }

    [Test]
    public void TestInstallFrameworkUpdate()
    {
      MockArchiveFile archive = new MockArchiveFile(null, "C:\\temp.zip");
      new MockArchiveFile(archive, "GamesManager.exe.config", "GamesManagerConfig");
      new MockArchiveFile(archive, "GamesManager.exe", "GamesManager");
      new MockArchiveFile(archive, "GamesManager.Common.dll", "GamesManagerCommon");
      var schemas = new MockArchiveFile(archive, "schemas");
      new MockArchiveFile(schemas, "game.xsd", "GameSchema");
      ((MockArchiveFactory)ArchiveFactory.Current).VirtualArchives.Add("C:\\temp.zip", archive);


      MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
      gamesManagerFile.WriteText("EXECUTABLE");
      gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 0);

      FrameworkUpdater updater = new FrameworkUpdater("C:\\temp.zip");
      updater.Start();

      Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.New.exe").Exists);
      Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe.config").Exists);
      Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.Common.dll").Exists);
      Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\schemas\\game.xsd").Exists);

      Assert.AreEqual(new Version(1, 0, 0, 0), FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe").AssemblyVersion);
    }
  }
}
