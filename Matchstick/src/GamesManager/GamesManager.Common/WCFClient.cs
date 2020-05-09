using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.Text;

namespace MGDF.GamesManager.Common
{
  public interface IWCFClient<TChannel> where TChannel : class
  {
    TReturn Use<TReturn>(Uri uri, Func<TChannel, TReturn> code);
  }
}
