using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View
{
    public interface ILoginView: IView
    {
        event EventHandler OnLogin;
        string DeveloperKey { get; set; }
        string SecretKey { get; set; }
        string GameSourceUrl { get; set; }
        bool RememberMe { get; set; }
        void ShowError(string error);
        bool ShowLoggingIn { set; }
    }
}
