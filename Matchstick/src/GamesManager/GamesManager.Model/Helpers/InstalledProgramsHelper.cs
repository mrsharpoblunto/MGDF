using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.Model.Helpers
{
    public class InstalledProgramsHelper
    {
        public static void AddToInstalledPrograms(IGame game)
        {
            var key = Registry.Current.CreateSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF"+Constants.InterfaceVersion+"_" + game.Uid);
            key.CreateValue("DisplayIcon",Constants.GameSystemIconFile(game.Uid));
            key.CreateValue("DisplayName",game.Name);
            key.CreateValue("URLInfoAbout",game.Homepage);
            key.CreateDWordValue("NoModify",1);
            key.CreateDWordValue("NoRepair",1);
            key.CreateValue("Publisher",game.DeveloperName);
            key.CreateValue("InstallLocation",Constants.GameDir(game.Uid));
            key.CreateValue("UninstallString", Constants.GamesManagerExecutable + " " + Constants.GamesManagerUninstallArguments(game.Uid));
            key.CreateValue("DisplayVersion",game.Version.ToString());
        }

        public static void RemoveFromInstalledPrograms(string uid)
        {
            if (Registry.Current.OpenSubKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF" + Constants.InterfaceVersion + "_" + uid) != null)
            {
                Registry.Current.DeleteKey(BaseRegistryKey.LocalMachine, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGDF" + Constants.InterfaceVersion + "_" + uid);
            }
        }
    }
}
