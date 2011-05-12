using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Web;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public static class FileServer
    {
        private static IFileServer _current;

        public static IFileServer Current
        {
            get
            {
                return _current;
            }
            set
            {
                _current = value;
            }
        }
    }
}