using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockIdentityGenerator : IIdentityGenerator
  {
    private byte _index;

    public Guid GenerateIdentity()
    {
      return new Guid(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _index++);
    }
  }
}