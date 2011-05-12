using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
    class CheckUninstallPresenter: PresenterBase<ICheckUninstallView>
    {
        public CheckUninstallPresenter(List<string> gameUids)
        {
            var list = new List<string>();
            foreach (var uid in gameUids)
            {
                if (FileSystem.Current.FileExists(Path.Combine(Constants.GameDir(uid), Constants.GameConfig)))
                {
                    IGame game = EntityFactory.Current.CreateGame(Path.Combine(Constants.GameDir(uid), Constants.GameConfig));
                    if (game.ErrorCollection.Count==0)
                    {
                        list.Add(game.Name);
                        continue;
                    }
                }
                list.Add(uid);
            }

            View.DependantGames = list;
            View.OnUninstall += (s, e) =>
                                    {
                                        Uninstall = true;
                                        CloseView();
                                    };
        }

        public bool Uninstall { get; private set; }
    }
}