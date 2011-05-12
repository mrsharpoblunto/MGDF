using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.MVP.Views
{
    interface ISendStatisticsView: IView
    {
        event EventHandler OnAllow;
        event EventHandler OnDeny;

        IGame Game { set; }
    }
}