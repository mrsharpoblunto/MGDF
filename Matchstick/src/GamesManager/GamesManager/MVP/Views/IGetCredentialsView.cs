using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.MVP.Views
{
  interface IGetCredentialsView : IView
  {
    event EventHandler OnOK;

    Game Game { set; }
    string Password { get; }
    string Username { get; }
  }
}