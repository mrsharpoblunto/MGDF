using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Newtonsoft.Json;

namespace MGDF.GamesManager.GameSource.Handlers
{
    //clients should look for service endpoint urls here before invoking services. This makes it easier to implement
    //alternative versions which don't have such WCF-centric endpoint urls (.svc) but still allowing service compatibility.
    public class ServicesHandler: IHttpHandler
    {
        public void ProcessRequest(HttpContext context)
        {
            context.Response.ContentType = "application/json";
            context.Response.Write(JsonConvert.SerializeObject(new { GameSourceService = "/games.svc",DeveloperService="/developer.svc"}));
            context.Response.StatusCode = 200;
        }

        public bool IsReusable
        {
            get { return true; }
        }
    }
}
