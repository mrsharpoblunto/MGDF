using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.MVP.Views
{
    interface ISendStatisticsView: IView
    {
        event EventHandler OnAllow;
        event EventHandler OnDeny;

        Game Game { set; }
    }
}