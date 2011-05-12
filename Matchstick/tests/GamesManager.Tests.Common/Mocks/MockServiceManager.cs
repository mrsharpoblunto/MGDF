using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockService: IService
    {
        public string Name { get; private set; }

        public MockService(string name)
        {
            Name = name;
        }

        public void Start()
        {
        }

        public void Stop()
        {
        }

        public void WaitForStatus(ServiceControllerStatus status)
        {
        }
    }

    public class MockServiceManager: IServiceManager
    {
        public MockServiceManager()
        {
            Services = new List<MockService>();
        }

        public readonly List<MockService> Services;

        public IService GetService(string name)
        {
            return Services.SingleOrDefault(s => s.Name == name);
        }
    }
}