using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public class ServicesHandler: IHttpHandler
    {
        public void ProcessRequest(HttpContext context)
        {
            context.Response.ContentType = "application/json";
            context.Response.Write(
@"{ 
    GameSourceService: ""/games.svc"",
    DeveloperService:""/developer.svc"",
}");
            context.Response.StatusCode = 200;
        }

        public bool IsReusable
        {
            get { return true; }
        }
    }
}
