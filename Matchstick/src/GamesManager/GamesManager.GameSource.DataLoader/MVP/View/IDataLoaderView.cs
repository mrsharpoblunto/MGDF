using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View
{
    public interface IDataLoaderView : IAddGameVersionViewBase
    {
        event EventHandler OnLogout;
        event EventHandler OnRefresh;

        event EventHandler<SelectGameEventArgs> OnSelectGame;
        event EventHandler<SelectGameEventArgs> OnDeleteGame;
        event EventHandler<SelectGameEventArgs> OnUpdateGame;

        event EventHandler<AddGameEventArgs> OnAddGame;
        event EventHandler<AddGameVersionEventArgs> OnAddGameVersion;
        event EventHandler<SelectGameVersionEventArgs> OnSelectGameVersion;
        event EventHandler<SelectGameVersionEventArgs> OnDeleteGameVersion;
        event EventHandler<SelectGameVersionEventArgs> OnUpdateGameVersion;

        List<Game> Games { set; }
        List<GameVersion> GameVersions { set; }
        string UpdateDescription { set; }
        string DownloadURL { set; }

        void ShowSelectedGame(bool visible);
        void ShowSelectedGameVersion(bool visible);
    }
}
