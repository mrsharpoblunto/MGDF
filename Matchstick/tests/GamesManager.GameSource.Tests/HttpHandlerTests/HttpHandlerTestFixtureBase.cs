using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.GameSource.Tests.Mocks;
using MGDF.GamesManager.ServerCommon;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.GameSource.Tests.HttpHandlerTests
{
    [TestFixture]
    public abstract class HttpHandlerTestFixtureBase<T> where T : IMockableHttpHandler,new()
    {
        protected IMockableHttpHandler Handler { get; private set; }

        [SetUp]
        public virtual void SetUp()
        {
            Handler = new T();
            Config.Current = new MockConfig();
            ServerContext.Current = new MockServerContext();
            TimeService.Current = new MockTimeService(new DateTime(2009, 10, 10));
            FileSystem.Current = new MockFileSystem();
            Logger.Current = new MockLogger();
            GameSourceRepository.Context = new RepositoryContext(() => new MockGameSourceRepository());
            FileServer.Current = new DefaultFileServer();

            InsertTestData();
        }

        [TearDown]
        public virtual void TearDown()
        {
            GameSourceRepository.DisposeCurrent();
        }

        protected abstract void InsertTestData();

        protected static byte[] GenerateDataBlock(int length)
        {
            byte[] data = new byte[length];
            for (int i=0;i<length;++i)
            {
                data[i] = (byte) 128;
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