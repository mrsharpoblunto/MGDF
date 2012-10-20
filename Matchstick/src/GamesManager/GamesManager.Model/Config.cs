using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model
{
    public interface IConfig
    {
        bool AutoRegisterOnUpdate { get; }
    }

    public class Config: IConfig
    {
        public static IConfig Current { get; set; }

        public bool AutoRegisterOnUpdate
        {
            get { return bool.Parse(ConfigurationManager.AppSettings["autoRegisterOnUpdate"]); }
        }
    }
}
