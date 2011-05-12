using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Web;
using MGDF.GamesManager.GameSource.Model.FileServers;

namespace MGDF.GamesManager.GameSource.Model.Configuration
{
    public class FileServerFactory
    {
        public static IFileServer Create()
        {
            Assembly providerAssembly = string.IsNullOrEmpty(Config.Current.FileServerAssembly) ? Assembly.GetExecutingAssembly() : Assembly.Load(Config.Current.FileServerAssembly);
            Type providerType = string.IsNullOrEmpty(Config.Current.FileServerType) ? typeof(DefaultFileServer) : providerAssembly.GetType(Config.Current.FileServerType);
            return (IFileServer)Activator.CreateInstance(providerType);
        }
    }
}