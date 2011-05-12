using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockLogger: ILogger 
    {
        public void Write(LogInfoLevel level, string message)
        {
        }

        public void Write(Exception ex, string message)
        {
        }
    }
}