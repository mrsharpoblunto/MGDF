using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.Model.Helpers
{
    public class InstalledProgramsHelper
    {
        public static void AddToInstalledPrograms(Game game)
        {
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF" + Resources.InterfaceVersion + "_" + game.Uid);
            key.CreateValue("DisplayIcon", Resources.GameSystemIconFile());
            key.CreateValue("DisplayName", game.Name);
            key.CreateValue("URLInfoAbout", game.Homepage);
            key.CreateDWordValue("NoModify", 1);
            key.CreateDWordValue("NoRepair", 1);
            key.CreateValue("Publisher", game.DeveloperName);
            key.CreateValue("InstallLocation", Resources.GameBaseDir);
            key.CreateValue("DisplayVersion", game.Version.ToString());
        }

        public static void RemoveFromInstalledPrograms(string uid)
        {
            var key = Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF" + Resources.InterfaceVersion + "_" + uid);
            if (key != null)
            {
                key.DeleteValue("DisplayIcon");
                key.DeleteValue("DisplayName");
                key.DeleteValue("URLInfoAbout");
                key.DeleteValue("NoModify");
                key.DeleteValue("NoRepair");
                key.DeleteValue("Publisher");
                key.DeleteValue("InstallLocation");
                key.DeleteValue("DisplayVersion");
            }
        }
    }
}