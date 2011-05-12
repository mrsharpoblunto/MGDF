using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
    public interface ISplashScreenView: IView
    {
        event EventHandler OnInstall;
        string Version { set; }
        string GameName { set; }
        string Developer { set; }
        string Description { set; }
        Image SplashImage { set; }
        Image GameIcon { set; }
    }
}