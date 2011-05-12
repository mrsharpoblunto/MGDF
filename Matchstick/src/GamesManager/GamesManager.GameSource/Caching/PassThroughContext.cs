using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace MGDF.GamesManager.GameSource.Caching
{
    public class PassThroughContext<T>
    {
        public T Context { get; set; }
    }
}