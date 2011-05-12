using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Caching;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.GameSource.Tests.Mocks;
using MGDF.GamesManager.ServerCommon;
using MGDF.GamesManager.StatisticsService.Model;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.GameSource.Tests.ServiceTests
{
    [TestFixture]
    public abstract class ServiceTestFixtureBase<T> where T : new()
    {
        protected T Service { get; private set; }

        [SetUp]
        public virtual void SetUp()
        {
            Service = new T();
            Config.Current = new MockConfig();
            ServerContext.Current = new MockServerContext();
            TimeService.Current = new MockTimeService(new DateTime(2009, 10, 10));
            FileSystem.Current = new MockFileSystem();
            Logger.Current = new MockLogger();
            FileServer.Current = new DefaultFileServer();

            GameSourceRepository.Context = new RepositoryContext(() => new MockGameSourceRepository());
            StatisticsServiceRepository.Context = new RepositoryContext(() => new MockStatisticsServiceRepository());
            InsertTestData();
        }

        [TearDown]
        public virtual void TearDown()
        {
            CNonceCache.Instance.Clear();
            GameSourceRepository.DisposeCurrent();
            StatisticsServiceRepository.DisposeCurrent();
        }

        protected abstract void InsertTestData();
    }
}