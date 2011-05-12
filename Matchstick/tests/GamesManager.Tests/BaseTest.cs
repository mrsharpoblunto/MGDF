using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Model.Services;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;
using File=MGDF.GamesManager.Common.Framework.File;

namespace MGDF.GamesManager.Tests
{
    public class BaseTest
    {
        [SetUp]
        public virtual void Setup()
        {
            //use the mock entity & view implementations
            EntityFactory.Current = new EntityFactory();
            //ViewFactory.Current = new MockViewFactory();
            Logger.Current = new MockLogger();
            FileSystem.Current = new MockFileSystem();
            EnvironmentSettings.Current = new MockEnvironmentSettings();
            TimeService.Current = new MockTimeService();
            Registry.Current = new MockRegistry();
            IdentityGenerator.Current = new MockIdentityGenerator();
            HttpRequestManager.Current = new MockHttpRequestManager();

            //managers relevant to installing and installing games.
            GameExplorer.Current = new MockGameExplorer();
            IconManager.Current = new MockIconManager();
            ProcessManager.Current = new MockProcessManager();
            ServiceManager.Current = new MockServiceManager();
            ShortcutManager.Current = new MockShortcutManager();

            ((MockRegistry)Registry.Current).AddFakeKey(BaseRegistryKey.LocalMachine, new MockRegistryKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));

            MockDirectory volumeRoot = MockFileSystem.AddVolumeRoot("C:");
            volumeRoot.AddDirectory("temp");
            MockDirectory documentsDir = volumeRoot.AddDirectory("Documents and Settings");
            documentsDir.AddDirectory("All Users").AddDirectory("Application Data");
            MockDirectory userDir = documentsDir.AddDirectory("user");
            userDir.AddDirectory("Local Settings").AddDirectory("Application Data");
            userDir.AddDirectory("desktop");
            userDir.AddDirectory("start menu");
            volumeRoot.AddDirectory("Documents and Settings").AddDirectory("All Users").AddDirectory("Application Data");

            MockDirectory schemasDir = volumeRoot.AddDirectory("program files").AddDirectory("MGDF").AddDirectory("schemas");
            schemasDir.AddFile("game.xsd", ReadTextFile("Schemas\\game.xsd"));
            schemasDir.AddFile("gameState.xsd", ReadTextFile("Schemas\\gameState.xsd"));
            schemasDir.AddFile("preferences.xsd", ReadTextFile("Schemas\\preferences.xsd"));
            schemasDir.AddFile("update.xsd", ReadTextFile("Schemas\\update.xsd"));

            Constants.CreateRequiredCommonDirectories();
        }

        protected static MockFileSystem MockFileSystem
        {
            get
            {
                return (MockFileSystem) FileSystem.Current;
            }
        }
        protected static string TestDataPath
        {
            get
            {
                return "GamesManager.Tests.Content\\";
            }
        }

        protected static string ReadTextFile(string filename)
        {
            string testDataPath = string.Format("GamesManager.Tests.Content\\{0}",filename);
            
            using (FileStream stream = new FileStream(testDataPath,FileMode.Open,FileAccess.Read))
            {
                using (TextReader reader = new StreamReader(stream))
                {
                    return reader.ReadToEnd();
                }
            }
        }

        protected static byte[] ReadBinaryFile(string filename)
        {
            string testDataPath = string.Format("GamesManager.Tests.Content\\{0}",filename);
            Console.WriteLine("Loading file " + testDataPath);
            
            using (FileStream stream = new FileStream(testDataPath,FileMode.Open,FileAccess.Read))
            {
                List<byte> content = new List<byte>();
                byte[] buffer = new byte[16384];
                int count = stream.Read(buffer, 0, 16384);
                while (count>0)
                {
                    byte[] temp = new byte[count];
                    Array.Copy(buffer,temp,count);
                    content.AddRange(temp);
                    count = stream.Read(buffer, 0, 16384);
                }

                return content.ToArray();
            }
        }

        [TearDown]
        public virtual void TearDown()
        {
            
        }

        protected static byte[] GenerateDataBlock(int length)
        {
            byte[] data = new byte[length];
            for (int i = 0; i < length; ++i)
            {
                data[i] = (byte)128;
            }
            return data;
        }

        protected static byte[] GenerateRandomDataBlock(int length)
        {
            Random random = new Random();
            byte[] data = new byte[length];
            random.NextBytes(data);
            return data;
        }

        protected static string GenerateMd5Hash(byte[] data)
        {
            using (HashAlgorithm hashAlg = MD5.Create())
            {
                byte[] hash = hashAlg.ComputeHash(data);
                return hash.ConvertToBase16();
            }
        }
    }
}
