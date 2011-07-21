using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Model;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockConfig: IConfig
    {
        public bool AutoRegisterOnUpdate
        {
            get { return true; }
        }

        public bool AutoUpdateFramework
        {
            get { return true; }
        }

        public string FrameworkUpdateSite
        {
            get { return "http://www.matchstickframework.org"; }
        }
    }
}
