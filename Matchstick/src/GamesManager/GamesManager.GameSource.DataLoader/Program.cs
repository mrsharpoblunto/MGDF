using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;

namespace MGDF.GamesManager.GameSource.DataLoader
{
    static class Program
    {

        [STAThread]
        static void Main(string[] args)
        {
            TimeService.Current = new TimeService();
            EnvironmentSettings.Current = new EnvironmentSettings();
            FileSystem.Current = new FileSystem();
            ArchiveFactory.Current = new ArchiveFactory();
            EntityFactory.Current = new EntityFactory();
            Logger.Current = new Logger(Path.Combine(EnvironmentSettings.Current.UserDirectory, "DataLoaderLog.txt"));
            HttpRequestManager.Current = new HttpRequestManager();
            Constants.CreateRequiredUserDirectories();

            ViewFactory.Current.RegisterView<ILoginView,LoginForm>();
            ViewFactory.Current.RegisterView<IDataLoaderView,DataLoaderForm>();
            ViewFactory.Current.RegisterView<IAddGameVersionView,AddGameVersionForm>();
            ViewFactory.Current.RegisterView<IUploadGameVersionView,UploadGameVersionForm>();

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            DataLoaderPresenter presenter = new DataLoaderPresenter();
            Application.Run(presenter.View as Form);
        }
    }
}
