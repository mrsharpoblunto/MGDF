using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Service
{
    static class Config
    {
        public static bool AutoUpdateFramework
        {
            get
            {
                return bool.Parse(ConfigurationManager.AppSettings["autoUpdateFramework"]);
            }
        }
    }
}