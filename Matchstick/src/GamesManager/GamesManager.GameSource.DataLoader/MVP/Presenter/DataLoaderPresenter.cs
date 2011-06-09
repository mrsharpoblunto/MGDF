using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;
using MGDF.GamesManager.Model.Factories;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter
{
    class DataLoaderPresenter: DataLoaderPresenterBase<IDataLoaderView>
    {
        public DataLoaderPresenter()
        {
            View.Shown += View_Shown;

            View.OnLogout += View_OnLogout;
            View.OnRefresh += View_OnRefresh;

            View.OnSelectGame += View_OnSelectGame;
            View.OnDeleteGame += View_OnDeleteGame;
            View.OnUpdateGame += View_OnEditGame;

            View.OnAddGame += View_OnAddGame;
            View.OnAddGameVersion += View_OnAddGameVersion;
            View.OnSelectGameVersion += View_OnSelectGameVersion;
            View.OnDeleteGameVersion += View_OnDeleteGameVersion;
            View.OnUpdateGameVersion += View_OnUpdateGameVersion;
        }

        void View_OnRefresh(object sender, EventArgs e)
        {
            InitializeView();
        }

        void View_OnUpdateGameVersion(object sender, SelectGameVersionEventArgs e)
        {
            var request = NewRequest<EditGameVersionRequest>();
            request.Id = e.GameVersion.Id;
            request.Published = View.Published;

            var response = DeveloperService(s=>s.EditGameVersion(request));
            if (response.Errors.Count() > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to edit game version: " + response.Errors[0].Message);
                Message.Show("Unable to edit game version: " + response.Errors[0].Message, "Unable to edit game version");
            }
        }

        void View_OnDeleteGameVersion(object sender, SelectGameVersionEventArgs e)
        {
            var request = NewRequest<DeleteGameVersionRequest>();
            request.Id = e.GameVersion.Id;

            var response = DeveloperService(s=>s.DeleteGameVersion(request));

            if (response.Errors.Count() > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to delete game version: " + response.Errors[0].Message);
                Message.Show("Unable to delete game version: " + response.Errors[0].Message, "Unable to delete game version");
            }
            else
            {
                GetGameVersions(e.Game);
            }
        }

        void View_OnSelectGameVersion(object sender, SelectGameVersionEventArgs e)
        {
            View.Version = new Version(e.GameVersion.Version);
            View.UpdateDescription = e.GameVersion.Description ?? string.Empty;
            View.Published = e.GameVersion.Published;
            View.DownloadURL = e.GameVersion.DownloadURL;
            View.UpdateVersions(e.GameVersion.IsUpdate, e.GameVersion.UpdateMinVersion, e.GameVersion.UpdateMaxVersion);
            View.ShowSelectedGameVersion(true);
        }

        void View_OnAddGame(object sender, AddGameEventArgs e)
        {
            var addGame = new AddGamePresenter(e.GameFile);
            addGame.OnComplete += addGame_OnComplete;
            addGame.OnError += addGame_OnComplete;
            addGame.ShowView(View);
        }

        void addGame_OnComplete(object sender, EventArgs e)
        {
            GetGames();
        }

        void View_OnAddGameVersion(object sender, AddGameVersionEventArgs e)
        {
            var addGame = new AddGameVersionPresenter(e.Game,e.GameFile);
            addGame.OnComplete += addGameVersion_OnComplete;
            addGame.OnError += addGame_OnComplete;
            addGame.ShowView(View);
        }

        void addGameVersion_OnComplete(object sender, SelectGameEventArgs e)
        {
            GetGameVersions(e.Game);
        }

        void View_OnEditGame(object sender, SelectGameEventArgs e)
        {
            var request = NewRequest<EditGameRequest>();
            request.EditGame = new EditGame { Uid = e.Game.Uid,RequiresAuthentication = View.RequiresAuthentication,RequiresAuthenticationSpecified = true};

            var response = DeveloperService(s=>s.EditGame(request));

            if (response.Errors.Count() > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to edit Game: " + response.Errors[0].Message);
                Message.Show("Unable to edit Game: " + response.Errors[0].Message, "Unable to edit Game");
            }
        }

        void View_OnDeleteGame(object sender, SelectGameEventArgs e)
        {
            var request = NewRequest<DeleteGameRequest>();
            request.GameUid = e.Game.Uid;
            
            var response = DeveloperService(s=>s.DeleteGame(request));

            if (response.Errors.Count() > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to delete Game: " + response.Errors[0].Message);
                Message.Show("Unable to delete Game: " + response.Errors[0].Message, "Unable to delete Game");
            }
            else
            {
                GetGames();
            }
        }

        void View_OnSelectGame(object sender, SelectGameEventArgs e)
        {
            View.Uid = e.Game.Uid;
            View.GameName = e.Game.Name;
            View.InterfaceVersion = e.Game.InterfaceVersion;
            View.Description = e.Game.Description;
            View.Homepage = e.Game.Homepage;

            View.ShowSelectedGame(true);
            View.ShowSelectedGameVersion(false);
            GetGameVersions(e.Game);
        }

        void View_OnLogout(object sender, EventArgs e)
        {
            Settings.Instance.Clear();
            CloseView();
        }

        void View_Shown(object sender, EventArgs e)
        {
            LoginPresenter presenter = new LoginPresenter();
            presenter.ShowView(View);

            if (Settings.Instance.Credentials==null)
            {
                CloseView();
            }
            else
            {
                InitializeView();
            }
        }

        private void InitializeView()
        {
            GetGames();
        }

        private void GetGames()
        {
            View.ShowSelectedGame(false);
            View.ShowSelectedGameVersion(false);

            var getGamesRequest = NewRequest<GetGamesRequest>();
            getGamesRequest.DeveloperUid = Settings.Instance.Credentials.Uid;
            getGamesRequest.InterfaceVersion = Constants.InterfaceVersion;

            var gamesResponse = DeveloperService(s=>s.GetGames(getGamesRequest));
            if (gamesResponse.Errors.Count > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to load games: " + gamesResponse.Errors[0].Message);
                Message.Show("Unable to load games: " + gamesResponse.Errors[0].Message, "Unable to load games");
            }
            else
            {
                View.Games = gamesResponse.Games;
            }
        }

        private void GetGameVersions(Game selectedGame)
        {
            View.ShowSelectedGameVersion(false);

            var request = NewRequest<GetGameVersionsRequest>();
            request.GameUid = selectedGame.Uid;

            var response = DeveloperService(s=>s.GetGameVersions(request));
            if (response.Errors.Count > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to load game versions: " + response.Errors[0].Message);
                Message.Show("Unable to load games: " + response.Errors[0].Message, "Unable to load game versions");
            }
            else
            {
                View.GameVersions = response.GameVersions;
            }
        }
    }
}
