using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Routing;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.ServerCommon;

namespace MatchstickFramework.Web
{
    // Note: For instructions on enabling IIS6 or IIS7 classic mode, 
    // visit http://go.microsoft.com/?LinkId=9394801

    public class MvcApplication : System.Web.HttpApplication
    {
        public static void RegisterRoutes(RouteCollection routes)
        {
            routes.IgnoreRoute("{resource}.axd/{*pathInfo}");

            routes.MapRoute(
                "Default",
                "LatestVersion/{id}",
                new { controller = "LatestVersion", action = "Index", id = "" } 
            );

        }

        protected void Application_Start()
        {
            FileSystem.Current = new FileSystem();
            ServerContext.Current = new ServerContext();
            TimeService.Current = new TimeService();
            Logger.Current = new MGDFLog4NetLogger();

            RegisterRoutes(RouteTable.Routes);
        }

        protected void Application_Error(object sender, EventArgs e)
        {
            Logger.Current.Write(Server.GetLastError(), "Unhandled error");
        }
    }
}