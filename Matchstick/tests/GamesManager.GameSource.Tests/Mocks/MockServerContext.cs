using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockServerContext: IServerContext 
    {
        public string MapPath(string path)
        {
            return Path.Combine("c:\\gamesource", path.TrimStart(new[] { '~','/','\\' }).Replace("/", "\\"));
        }
    }
}