using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockWCFClient<TCHannel>:IWCFClient<TCHannel>
    {
        private readonly TCHannel _service;

        public MockWCFClient(TCHannel service)
        {
            _service = service;
        }

        public TReturn Use<TReturn>(Func<TCHannel, TReturn> code)
        {
            return code(_service);
        }
    }
}
