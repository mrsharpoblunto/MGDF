using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Newtonsoft.Json;

namespace MGDF.GamesManager.GameSource.Handlers
{
    //clients should look for service endpoint urls here before invoking services. This makes it easier to implement
    //alternative versions which don't have such WCF-centric endpoint urls (.svc) but still allowing service compatibility.
    public class ServicesModule: IHttpModule
    {
        public void Init(HttpApplication context)
        {
            context.BeginRequest += context_BeginRequest;
        }

        private static void context_BeginRequest(object sender, EventArgs e)
        {
            var context = HttpContext.Current;
            if (context.Request.Url.PathAndQuery.Equals("/Services", StringComparison.InvariantCultureIgnoreCase))
            {
                context.Response.ContentType = "application/json";
                context.Response.Write(JsonConvert.SerializeObject(new { GameSourceService = "/games.svc", DeveloperService = "/developer.svc" }));
                context.Response.StatusCode = 200;
                context.Response.End();
            }
        }

        public void Dispose()
        {
        }
    }
}
