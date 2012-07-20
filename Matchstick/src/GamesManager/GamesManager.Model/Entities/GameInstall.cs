using System;
using System.Collections.Generic;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model.Entities
{
    public class GameInstall: IDisposable
    {
        public GameInstall(string installerFile)
        {
            ErrorCollection = new List<string>();
            try
            {
                InstallerFile = installerFile;
                LoadContent(installerFile);
                ValidateGameStructure();

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

        public Game Game { get; private set; }
        public Update Update { get; private set; }
        public IArchiveFile GameContents { get; private set; }
        public string InstallerFile { get; private set; }
        public bool IsUpdate { get; private set; }
        public List<string> ErrorCollection { get; private set; }

        public bool IsValid
        {
            get { return ErrorCollection.Count == 0; }
        }

        private void ValidateGameStructure()
        {
            if (GameContents.GetFile(Resources.GameConfig) == null)
            {
                ErrorCollection.Add("No Game configuration found (expected \"" + Resources.GameConfig + "\")");
            }

            IsUpdate = GameContents.GetFile(Resources.UpdateConfig) != null;

            if (!IsUpdate && GameContents.GetFile(Resources.ContentDir) == null)
            {
                ErrorCollection.Add("No content directory found");
            }

            if (!IsUpdate && GameContents.GetFile(Resources.BinDir) == null)
            {
                ErrorCollection.Add("No bin directory found");
            }
        }

        private void Load()
        {
            Game = new Game(GameContents.GetFile(Resources.GameConfig));
            ErrorCollection.AddRange(Game.ErrorCollection);

            if (GameContents.GetFile(Resources.UpdateConfig) != null)
            {
                Update = new Update(Game,GameContents.GetFile(Resources.UpdateConfig));
                ErrorCollection.AddRange(Update.ErrorCollection);
            }
        }

        public void Dispose()
        {
            if (GameContents!=null) GameContents.Dispose();
        }
    }
}