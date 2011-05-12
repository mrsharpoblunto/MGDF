using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
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
    abstract class AddGamePresenterBase: DataLoaderPresenterBase<IAddGameVersionView>
    {
        public event EventHandler<SelectGameEventArgs> OnComplete;
        public event EventHandler<SelectGameEventArgs> OnError;

        private readonly IGameInstall _installer;
        private readonly string _gameFile;
        private UploadGamePresenter _uploader;

        public AddGamePresenterBase(string GameFile)
        {
            _gameFile = GameFile;
            _installer = EntityFactory.Current.CreateGameInstall(GameFile);

            View.Shown += View_Shown;
            View.OnAddGame += View_OnAddGame;
            View.Closed += View_Closed;
        }

        void View_Closed(object sender, EventArgs e)
        {
            if (_installer != null && _installer.GameContents != null)
            {
                _installer.GameContents.Dispose();
            }
        }

        protected abstract void UpdateView(IGameInstall installer);
        protected abstract bool ValidateInstaller(IGameInstall installer);
        protected abstract bool AddGame(IGameInstall installer);
        protected abstract SelectGameEventArgs GetCompletionEventArgs();

        void View_Shown(object sender, EventArgs e)
        {
            if (ValidateInstaller(_installer))
            {
                View.GameName = _installer.Game.Name;

                if (_installer.IsUpdate)
                {
                    View.Description = _installer.Update.UpdateDescription;
                    View.UpdateVersions(true, _installer.Update.UpdateMinVersion.ToString(), _installer.Update.UpdateMaxVersion.ToString());
                }
                else
                {
                    View.Description = _installer.Game.Description;
                    View.UpdateVersions(false, null, null);
                }

                View.Published = false;
                View.RequiresAuthentication = false;
                View.Uid = _installer.Game.Uid;
                View.Version = _installer.Game.Version;
                View.InterfaceVersion = _installer.Game.InterfaceVersion;
                View.Homepage = _installer.Game.Homepage;

                if (_installer.Game.GameIconData!=null)
                {
                    View.GameIconData = _installer.Game.GameIconData;
                }

                UpdateView(_installer);
            }
            else
            {
                CloseView();
            }
        }

        private void View_OnAddGame(object sender, EventArgs e)
        {
            if (AddGame(_installer))
            {

                //add the gameversion.
                var addGameVersionRequest = RequestBuilder.Build<AddGameVersionRequest>();
                addGameVersionRequest.GameUid = _installer.Game.Uid;
                addGameVersionRequest.PublishOnUploadComplete = View.Published;
                addGameVersionRequest.NewGameVersion = new GameVersionBase
                {
                    IsUpdate = _installer.IsUpdate,
                    Md5Hash = FileSystem.Current.GetFile(_gameFile).ComputeMD5(),
                    Version = _installer.Game.Version.ToString()
                };

                if (_installer.IsUpdate)
                {
                    addGameVersionRequest.NewGameVersion.Description = _installer.Update.UpdateDescription;
                    addGameVersionRequest.NewGameVersion.UpdateMinVersion = _installer.Update.UpdateMinVersion.ToString();
                    addGameVersionRequest.NewGameVersion.UpdateMaxVersion = _installer.Update.UpdateMaxVersion.ToString();
                }

                var addGameVersionResponse = DeveloperService.AddGameVersion(addGameVersionRequest);
                if (addGameVersionResponse.Errors.Count > 0)
                {
                    CleanupInvalidVersion(_installer,null);
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to add game version: " + addGameVersionResponse.Errors[0].Message);
                    Message.Show("Unable to add game version: " + addGameVersionResponse.Errors[0].Message, "Unable to add game version ");
                    CloseView();
                }
                else
                {
                    //now upload the game
                    _uploader = new UploadGamePresenter(_gameFile, addGameVersionResponse.GameFragmentId, addGameVersionResponse.UploadHandler,addGameVersionResponse.MaxUploadPartSize);
                    _uploader.OnError += uploader_OnError;
                    _uploader.OnComplete += uploader_OnComplete;
                    _uploader.OnCancelled += uploader_OnCancelled;
                    _uploader.ShowView(View);
                    CloseView();
                }
            }
            else
            {
                CloseView();
            }
        }

        void uploader_OnError(object sender, EventArgs e)
        {
            CleanupInvalidVersion(_installer,_uploader);
            Logger.Current.Write(LogInfoLevel.Error, "Unable to upload game version");
            Message.Show("Unable to add game version", "Unable to add game version ");
            if (OnError != null)
            {
                OnError(this, GetCompletionEventArgs());
            }
        }

        protected abstract void CleanupInvalidVersion(IGameInstall installer,UploadGamePresenter uploader);

        void uploader_OnCancelled(object sender, EventArgs e)
        {
            CleanupInvalidVersion(_installer, _uploader);
        }

        void uploader_OnComplete(object sender, EventArgs e)
        {
            if (OnComplete != null)
            {
                OnComplete(this, GetCompletionEventArgs());
            }
        }
    }
}
