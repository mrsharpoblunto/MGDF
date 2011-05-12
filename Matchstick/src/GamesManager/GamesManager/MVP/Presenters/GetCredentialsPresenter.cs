using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class GetCredentialsPresenter: PresenterBase<IGetCredentialsView>
    {
        private readonly GetCredentialsEventArgs _args;

        public bool OK { get; private set; }

        public GetCredentialsPresenter(IGame game, GetCredentialsEventArgs args)
        {
            OK = false;
            _args = args;
            View.Game = game;
            View.OnOK += View_OnOK;
        }

        void View_OnOK(object sender, EventArgs e)
        {
            OK = true;
            _args.UserName = View.Username.ToLowerInvariant();
            _args.Password = View.Password;
            CloseView();
        }
    }
}