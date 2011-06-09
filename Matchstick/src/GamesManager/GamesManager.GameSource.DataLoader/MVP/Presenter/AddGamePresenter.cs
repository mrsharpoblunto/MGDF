using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
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
    class AddGamePresenter: AddGamePresenterBase
    {
        public AddGamePresenter(string gameFile) : base(gameFile)
        {
        }

        protected override void UpdateView(IGameInstall installer)
        {
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
            else if (installer.IsUpdate)
            {
                Logger.Current.Write(LogInfoLevel.Error, "The first version for a game cannot be an update GameVersion: " + installer.Update.ErrorCollection[0]);
                ViewFactory.Current.CreateView<IMessage>().Show(installer.Update.ErrorCollection[0], "Unable to add update GameVersion");
                return false;
            }
            return true;
        }

        protected override bool AddGame(IGameInstall installer)
        {
            var getGamesRequest = NewRequest<GetGamesRequest>();
            getGamesRequest.GameUid = installer.Game.Uid;
            getGamesRequest.InterfaceVersion = Constants.InterfaceVersion;

            Game existingGame = DeveloperService(s=>s.GetGames(getGamesRequest).Games.SingleOrDefault());
            if (existingGame != null)
            {
                Logger.Current.Write(LogInfoLevel.Error, "This game already exists");
                Message.Show("Unable to add game: " + installer.Game.Uid, "Game already exists");
                return false;
            }
            else
            {
                var addGameRequest = NewRequest<AddGameRequest>();
                addGameRequest.NewGame = new EditGame
                {
                    Description = installer.Game.Description,
                    Homepage = installer.Game.Homepage,
                    InterfaceVersion = installer.Game.InterfaceVersion,
                    Name = installer.Game.Name,
                    Uid = installer.Game.Uid,
                    RequiresAuthentication = View.RequiresAuthentication,
                    RequiresAuthenticationSpecified = true
                };
                var response = DeveloperService(s=>s.AddGame(addGameRequest));
                if (response.Errors.Count > 0)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to add game: " + response.Errors[0].Message);
                    Message.Show("Unable to add game: " + response.Errors[0].Message, "Unable to add game");
                    return false;
                }
            }
            return true;
        }

        protected override void CleanupInvalidVersion(IGameInstall installer, UploadGamePresenter uploader)
        {
            var deleteGameRequest = RequestBuilder.Build<DeleteGameRequest>();
            deleteGameRequest.GameUid = installer.Game.Uid;
            DeveloperService(s=>s.DeleteGame(deleteGameRequest));
        }

        protected override SelectGameEventArgs GetCompletionEventArgs()
        {
            return new SelectGameEventArgs();
        }
    }
}
