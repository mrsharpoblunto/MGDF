using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.MVP.Views
{
    interface IGetCredentialsView: IView
    {
        event EventHandler OnOK;

        IGame Game { set; }
        string Password { get; }
        string Username { get; }
    }
}