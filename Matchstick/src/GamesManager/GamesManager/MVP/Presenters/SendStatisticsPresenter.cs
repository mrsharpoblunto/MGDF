using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class SendStatisticsPresenter: PresenterBase<ISendStatisticsView>
    {
        public SendStatisticsPresenter(IGame game)
        {
            UserPermissionGranted = false;
            View.Game = game;
            View.OnAllow += (s, e) =>
                                {
                                    UserPermissionGranted = true;
                                    CloseView();
                                };
            View.OnDeny += (s, e) => CloseView();
        }

        public bool UserPermissionGranted { get; private set; }
    }
}