using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class FrameworkUpdateTests: BaseTest
    {
        [TearDown]
        public override void TearDown()
        {
            FrameworkUpdateManager.Instance.Dispose();
            base.TearDown();
        }

        [Test]
        public void TestFrameworkUpdaterExeGetsUpdated()
        {
            MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
            gamesManagerFile.WriteText("EXECUTABLE");
            gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 1);

            MockFile gamesManagerUpdaterFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.FrameworkUpdater.exe");
            gamesManagerUpdaterFile.WriteText("EXECUTABLE");
            gamesManagerUpdaterFile.AssemblyVersion = new Version(1, 0, 0, 1);


            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.1.2.4</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.0.0.1.exe</url>
</latestversion>");

            FrameworkUpdateManager.Instance.Start();

            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.FrameworkUpdater.exe").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.FrameworkUpdater.Current.exe").Exists);
        }

        [Test]
        public void TestFrameworkUpdater()
        {
            MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
            gamesManagerFile.WriteText("EXECUTABLE");
            gamesManagerFile.AssemblyVersion = new Version(1,0,0,1);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion/1", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.1.2.4</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</url>
</latestversion>");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe", GenerateDataBlock(55000));

            bool finished = false;
            FrameworkUpdateManager.Instance.OnComplete += (s, e) => finished = true;
            FrameworkUpdateManager.Instance.Start();
            while (!finished)
            {
                Thread.Sleep(0);
            }

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.exe").Exists);
        }

        [Test]
        public void TestFrameworkUpdaterError()
        {
            MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
            gamesManagerFile.WriteText("EXECUTABLE");
            gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 1);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion/1", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.1.2.4</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</url>
</latestversion>");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe", GenerateDataBlock(55000),
                (s,e)=>{ throw new Exception("Connection error");},null);

            bool finished = false;
            FrameworkUpdateManager.Instance.OnComplete += (s, e) => finished = true;
            FrameworkUpdateManager.Instance.Start();
            while (!finished)
            {
                Thread.Sleep(0);
            }


            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.exe").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.part").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\Framework\\pendingframeworkdownload.xml").Exists);

        }

        [Test]
        public void TestFrameworkUpdaterShutDownAndRestart()
        {
            MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
            gamesManagerFile.WriteText("EXECUTABLE");
            gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 1);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion/1", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.1.2.4</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</url>
</latestversion>");

            bool doExit=true;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe", GenerateDataBlock(55000),                (s, e) =>
                {
                    if (doExit)
                    {
                        FrameworkUpdateManager.Instance.Pause();
                        doExit = false;
                    }
                }, null);

            //service started
            bool started = false;
            FrameworkUpdateManager.Instance.OnStarted += (s, e) => started = true;
            FrameworkUpdateManager.Instance.Start();
            while (!started)
            {
                Thread.Sleep(0);
            }

            //service stopped
            while (doExit)
            {
                Thread.Sleep(0);
            }
            FrameworkUpdateManager.Instance.Dispose();

            //check that the queue file was persisted
            var queueFile = FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\Framework\\pendingframeworkdownload.xml");
            string queueFileContents = queueFile.ReadText();
            string expectedQueueFile =
                @"﻿<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
"<pendingframeworkdownload>"+"\r\n"+
"	<status>Paused</status>"+"\r\n"+
"	<progress>16384</progress>"+"\r\n"+
"	<total>55000</total>"+"\r\n"+
@"	<destinationfilename>c:\program files\MGDF\games\Downloads\Framework\1.1.2.4.part</destinationfilename>"+"\r\n"+
"	<sourceurl>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</sourceurl>"+"\r\n"+
"	<version>1.1.2.4</version>"+"\r\n"+
"</pendingframeworkdownload>";
            Assert.AreEqual(expectedQueueFile, queueFileContents);

            //service restarted
            bool finished = false;
            FrameworkUpdateManager.Instance.OnComplete += (s, e) => finished = true;
            FrameworkUpdateManager.Instance.Start();

            while (!finished)
            {
                Thread.Sleep(0);
            }

            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.exe").Exists);
            Assert.AreEqual(55000, FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.exe").Length);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\Framework\\pendingframeworkdownload.xml").Exists);
        }

        [Test]
        public void TestFrameworkUpdaterShutDownAndRestartWithLaterVersionAvailable()
        {
            MockFile gamesManagerFile = (MockFile)FileSystem.Current.GetFile("C:\\program files\\MGDF\\GamesManager.exe");
            gamesManagerFile.WriteText("EXECUTABLE");
            gamesManagerFile.AssemblyVersion = new Version(1, 0, 0, 1);

            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion/1", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.1.2.4</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</url>
</latestversion>");

            bool doExit = true;
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe", GenerateDataBlock(55000), (s, e) =>
            {
                if (doExit)
                {
                    FrameworkUpdateManager.Instance.Pause();
                    doExit = false;
                }
            }, null);

            //service started
            bool started = false;
            FrameworkUpdateManager.Instance.OnStarted += (s, e) => started = true;
            FrameworkUpdateManager.Instance.Start();
            while (!started)
            {
                Thread.Sleep(0);
            }

            //service stopped
            while (doExit)
            {
                Thread.Sleep(0);
            }
            FrameworkUpdateManager.Instance.Dispose();

            //check that the queue file was persisted
            var queueFile = FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\Framework\\pendingframeworkdownload.xml");
            string queueFileContents = queueFile.ReadText();
            string expectedQueueFile =
                @"﻿<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
"<pendingframeworkdownload>"+"\r\n"+
"	<status>Paused</status>"+"\r\n"+
"	<progress>16384</progress>"+"\r\n"+
"	<total>55000</total>"+"\r\n"+
@"	<destinationfilename>c:\program files\MGDF\games\Downloads\Framework\1.1.2.4.part</destinationfilename>"+"\r\n"+
"	<sourceurl>http://www.matchstickframework.org/downloads/MGDF-1.1.2.4.exe</sourceurl>"+"\r\n"+
"	<version>1.1.2.4</version>"+"\r\n"+
"</pendingframeworkdownload>";
            Assert.AreEqual(expectedQueueFile, queueFileContents);

            //simulate a newer version coming available
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/latestVersion/1", @"<?xml version=""1.0"" encoding=""utf-8"" ?>
<latestversion>
    <version>1.2.0.0</version>
    <url>http://www.matchstickframework.org/downloads/MGDF-1.2.0.0.exe</url>
</latestversion>");
            ((MockHttpRequestManager)HttpRequestManager.Current).ExpectResponse("http://www.matchstickframework.org/downloads/MGDF-1.2.0.0.exe", GenerateDataBlock(155000));

            //now restart
            bool finished = false;
            FrameworkUpdateManager.Instance.OnComplete += (s, e) => finished = true;
            FrameworkUpdateManager.Instance.Start();
            while (!finished)
            {
                Thread.Sleep(0);
            }

            //should have ditched the incomplete vesion and started on the newer one.
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.exe").Exists);
            Assert.IsFalse(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.1.2.4.part").Exists);
            Assert.IsTrue(FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.2.0.0.exe").Exists);
            Assert.AreEqual(155000, FileSystem.Current.GetFile("c:\\program files\\MGDF\\games\\Downloads\\Framework\\1.2.0.0.exe").Length);
            Assert.IsFalse(FileSystem.Current.GetFile("C:\\program files\\MGDF\\games\\Downloads\\Framework\\pendingframeworkdownload.xml").Exists);
        }
    }
}
