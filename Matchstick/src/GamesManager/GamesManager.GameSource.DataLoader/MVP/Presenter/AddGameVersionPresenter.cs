using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter
{
    class AddGameVersionPresenter : AddGamePresenterBase
    {
        private readonly Game _game;


        public AddGameVersionPresenter(Game game, string gameFile)
            : base(gameFile)
        {
            _game = game;
        }

        protected override void UpdateView(IGameInstall installer)
        {
            View.RequiresAuthentication = _game.RequiresAuthentication;
        }

        protected override bool ValidateInstaller(IGameInstall installer)
        {
            if (installer.ErrorCollection.Count > 0)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Invalid Game file: " + installer.ErrorCollection[0]);
                ViewFactory.Current.CreateView<IMessage>().Show(installer.ErrorCollection[0], "Invalid Game file");
                return false;
            }
            else if (!installer.Game.IsValid)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Invalid Game definition: " + installer.Game.ErrorCollection[0]);
                ViewFactory.Current.CreateView<IMessage>().Show(installer.Game.ErrorCollection[0], "Invalid Game definition");
                return false;
            }
            else if (installer.IsUpdate && !installer.Update.IsValid)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Invalid Game update definition: " + installer.Update.ErrorCollection[0]);
                ViewFactory.Current.CreateView<IMessage>().Show(installer.Update.ErrorCollection[0], "Invalid Game update definition");
                return false;
            }
            else if (_game != null && _game.Uid != installer.Game.Uid)
            {
                Logger.Current.Write(LogInfoLevel.Error, "This is not a version of the selected game");
                ViewFactory.Current.CreateView<IMessage>().Show("This is not a version of the selected game", "Wrong game");
                return false;
            }
            return true;
        }

        protected override bool AddGame(IGameInstall installer)
        {
            Game existingGame = GamesService.GetGames(new GetGamesRequest { GameUid = installer.Game.Uid, InterfaceVersion = Constants.InterfaceVersion }).Games.SingleOrDefault();
            if (existingGame != null)
            {
                var editGameRequest = RequestBuilder.Build<EditGameRequest>();
                editGameRequest.EditGame = new EditGame
                {
                    Description = installer.Game.Description,
                    Homepage = installer.Game.Homepage,
                    InterfaceVersion = installer.Game.InterfaceVersion,
                    Name = installer.Game.Name,
                    Uid = installer.Game.Uid,
                    RequiresAuthentication = View.RequiresAuthentication,
                    RequiresAuthenticationSpecified = true
                };
                var response = DeveloperService.EditGame(editGameRequest);
                if (response.Errors.Count > 0)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to edit game: " + response.Errors[0].Message);
                    Message.Show("Unable to edit game: " + response.Errors[0].Message, "Unable to edit game");
                    return false;
                }
            }
            else
            {
                Logger.Current.Write(LogInfoLevel.Error, "This game does not exist");
                Message.Show("Unable to edit game: " + installer.Game.Uid, "Game does not exist");
                return false;
            }
            return true;
        }

        protected override SelectGameEventArgs GetCompletionEventArgs()
        {
            return new SelectGameEventArgs{Game = _game};
        }

        protected override void CleanupInvalidVersion(IGameInstall installer,UploadGamePresenter uploader)
        {
            if (uploader != null && uploader.FragmentId != Guid.Empty)
            {
                var request = RequestBuilder.Build<CleanupIncompleteGameVersionRequest>();
                request.FragmentId = uploader.FragmentId;
                DeveloperService.CleanupIncompleteGameVersion(request);
            }
        }
    }
}
