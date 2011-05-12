using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
    interface ICheckUninstallView: IView
    {
        event EventHandler OnUninstall;
        List<string> DependantGames { set; }
    }
}