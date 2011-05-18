using System;
using System.Collections.Generic;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Factories;
using ArchiveFactory=MGDF.GamesManager.Model.Factories.ArchiveFactory;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Entities
{
    internal class GameInstall: IGameInstall
    {
        public GameInstall(string installerFile)
        {
            ErrorCollection = new List<string>();
            try
            {
                InstallerFile = installerFile;
                LoadContent(installerFile);
                ValidateGameStructure();
                ValidateGameContents();

                if (ErrorCollection.Count == 0)
                {
                    Load();
                }
            }
            catch (Exception e)
            {
                ErrorCollection.Add("Unexpected Game Error: " + e.Message);
            }
        }

        private void LoadContent(string installGame)
        {
            GameContents = ArchiveFactory.Current.OpenArchive(installGame);
        }

        #region IGameInstall Members

        public IGame Game { get; private set; }
        public IUpdate Update { get; private set; }
        public IArchiveFile GameContents { get; private set; }
        public string InstallerFile { get; private set; }
        public bool IsUpdate { get; private set; }
        public List<string> ErrorCollection { get; private set; }

        public bool IsValid
        {
            get { return ErrorCollection.Count == 0; }
        }

        #endregion

        private void ValidateGameStructure()
        {
            if (GameContents.GetFile(Constants.GameConfig) == null)
            {
                ErrorCollection.Add("No Game configuration found (expected \"" + Constants.GameConfig + "\")");
            }

            IsUpdate = GameContents.GetFile(Constants.UpdateConfig) != null;

            if (!IsUpdate && GameContents.GetFile(Constants.ContentDir) == null)
            {
                ErrorCollection.Add("No content directory found");
            }

            if (!IsUpdate && GameContents.GetFile(Constants.BinDir) == null)
            {
                ErrorCollection.Add("No bin directory found");
            }
        }

        private void ValidateGameContents()
        {
            XmlValidator validator = new XmlValidator();
            if (GameContents.GetFile(Constants.GameConfig) != null)
            {
                validator.IsXmlFileValid(GameContents.GetFile(Constants.GameConfig), "game.xsd", ErrorCollection);
            }
            if (GameContents.GetFile(Constants.UpdateConfig) != null)
            {
                validator.IsXmlFileValid(GameContents.GetFile(Constants.UpdateConfig), "update.xsd", ErrorCollection);
            }
            if (GameContents.GetFile(Constants.PreferencesConfig) != null)
            {
                validator.IsXmlFileValid(GameContents.GetFile(Constants.PreferencesConfig), "preferences.xsd", ErrorCollection);
            }
        }

        private void Load()
        {
            Game = new Game(GameContents.GetFile(Constants.GameConfig));
            ErrorCollection.AddRange(Game.ErrorCollection);

            if (GameContents.GetFile(Constants.UpdateConfig) != null)
            {
                Update = new Update(Game,GameContents.GetFile(Constants.UpdateConfig));
                ErrorCollection.AddRange(Update.ErrorCollection);
            }
        }
    }
}
