using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    public class InstallSplashScreenPresenter: PresenterBase<ISplashScreenView>
    {
        public IGameInstall Installer { get; private set;}
        public bool DoInstall { get; private set; }

        public InstallSplashScreenPresenter(string installerFile)
        {
            try
            {
                Installer = EntityFactory.Current.CreateGameInstall(installerFile);

                View.OnInstall += View_OnInstall;
                View.GameName = Installer.Game.Name;
                View.Developer = Installer.Game.DeveloperName;
                View.Description = Installer.Game.Description;
                View.Version = Installer.Game.Version.ToString();
                IArchiveFile splashImage = Installer.GameContents.GetFile(Constants.GameSplashImage);
                if (splashImage != null)
                {
                    using (var stream = splashImage.OpenStream())
                    {
                        View.SplashImage = Image.FromStream(stream);
                    }
                }
                if (Installer.Game.GameIconData != null)
                {
                    View.GameIcon = Image.FromStream(new MemoryStream(Installer.Game.GameIconData));
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unexpected error loading game information from installer package '" + installerFile + "'");
                Message.Show("Unexpected error loading game information from installer package", "Error loading installer package");
                Installer = null;
            }
        }

        private void View_OnInstall(object sender, EventArgs e)
        {
            DoInstall = true;
            CloseView();
        }
    }
}