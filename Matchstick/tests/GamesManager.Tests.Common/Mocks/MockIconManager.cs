using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockIconManager:IIconManager
    {
        public void CreateIcon(string name, string source, string destination)
        {
            FileSystem.Current.GetFile(destination).WriteText("ICON");
        }
    }
}