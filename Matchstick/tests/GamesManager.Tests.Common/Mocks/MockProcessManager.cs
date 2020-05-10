using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Model;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockProcessManager : IProcessManager
  {
    public void StartProcess(string filePath, string args, ProcessExited callback, object context)
    {
    }

    public int WaitForProcess(string filePath, string args)
    {
      return 0;
    }
  }
}