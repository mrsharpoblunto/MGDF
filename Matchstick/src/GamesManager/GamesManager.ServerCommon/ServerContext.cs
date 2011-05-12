using System;
using System.Collections.Generic;
using System.Text;
using System.Web;
using System.Web.Hosting;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.ServerCommon
{
    public interface IServerContext
    {
        string MapPath(string path);
    }

    public class ServerContext : IServerContext
    {
        public static IServerContext Current
        {
            get
            {
                return ServiceLocator.Current.Get<IServerContext>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        private HttpContext _context = null;

        public ServerContext()
        {
        }

        public ServerContext(HttpContext context)
        {
            _context = context;
        }

        public string MapPath(string path)
        {
            if (_context==null && HttpContext.Current==null)
            {
                return HostingEnvironment.MapPath(path);
            }
            else
            {
                return (_context ?? HttpContext.Current).Server.MapPath(path);
            }
        }
    }
}