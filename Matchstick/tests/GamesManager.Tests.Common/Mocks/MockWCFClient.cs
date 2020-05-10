using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockWCFClient<TChannel> : IWCFClient<TChannel> where TChannel : class
  {
    private readonly TChannel _service;

    public MockWCFClient(TChannel service)
    {
      _service = service;
    }

    public TReturn Use<TReturn>(Uri uri, Func<TChannel, TReturn> code)
    {
      return code(_service);
    }
  }
}
