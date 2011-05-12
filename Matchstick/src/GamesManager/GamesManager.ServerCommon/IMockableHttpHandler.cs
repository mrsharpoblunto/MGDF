using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.ServerCommon
{
    public interface IMockableHttpHandler
    {
        void ProcessRequest(IHttpRequest request, IHttpResponse response);
    }
}