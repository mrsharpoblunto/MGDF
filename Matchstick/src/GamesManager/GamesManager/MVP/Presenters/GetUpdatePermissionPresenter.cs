using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class GetUpdatePermissionPresenter: PresenterBase<IGetUpdatePermissionView>
    {
        public bool Update { get; private set; }

        public GetUpdatePermissionPresenter()
        {
            View.Game = Game.Current;
            View.OnUpdate += View_OnUpdate;
        }

        void View_OnUpdate(object sender, EventArgs e)
        {
            Update = true;
            CloseView();
        }
    }
}
