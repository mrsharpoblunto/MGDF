using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Web;
using System.Web.Security;
using System.Web.SessionState;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.ServerCommon;
using MGDF.GamesManager.StatisticsService.Model;
using MGDF.GamesManager.StatisticsService.Model.Configuration;

namespace MGDF.GamesManager.StatisticsService
{
    public class Global : System.Web.HttpApplication
    {
        protected void Application_Start(object sender, EventArgs e)
        {
            FileSystem.Current = new FileSystem();
            ServerContext.Current = new ServerContext();
            TimeService.Current = new TimeService();
            Logger.Current = new MGDFLog4NetLogger();
            StatisticsServiceRepository.Context = new WcfRepositoryContext(typeof(StatisticsServiceRepository), () => new StatisticsServiceRepository(Config.Current.ConnectionString));
        }

        protected void Application_BeginRequest(object sender, EventArgs e)
        {
        }

        protected void Application_EndRequest(object sender, EventArgs e)
        {
            StatisticsServiceRepository.DisposeCurrent();
        }

        protected void Application_Error(object sender, EventArgs e)
        {
            Logger.Current.Write(Server.GetLastError(), "Unhandled error");
        }
    }
}