using System;
using System.Collections;
using System.Configuration;
using System.Data;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.Threading;
using System.Web;
using System.Web.Security;
using System.Web.SessionState;
using System.Xml.Linq;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.GameSource.Model.FileServers;
using MGDF.GamesManager.ServerCommon;
using Directory=System.IO.Directory;
using File=System.IO.File;

namespace MGDF.GamesManager.GameSource
{
    public class Global : HttpApplication
    {
        protected void Application_Start(object sender,EventArgs e)
        {
            FileSystem.Current = new FileSystem();
            ServerContext.Current = new ServerContext();
            TimeService.Current = new TimeService();

            FileServer.Current = FileServerFactory.Create();

            Logger.Current = new MGDFLog4NetLogger();
            GameSourceRepository.Context = new WcfRepositoryContext(typeof(GameSourceRepository), () => new GameSourceRepository(Config.Current.ConnectionString));

            //ensures that deleted files and old incomplete fragments get cleaned up
            Janitor.Instance.Start();
        }

        protected void Application_BeginRequest(object sender, EventArgs e)
        {
        }

        protected void Application_EndRequest(object sender, EventArgs e)
        {
            GameSourceRepository.DisposeCurrent();
        }

        protected void Application_Error(object sender, EventArgs e)
        {
            Logger.Current.Write(Server.GetLastError(), "Unhandled error");
        }
    }

}