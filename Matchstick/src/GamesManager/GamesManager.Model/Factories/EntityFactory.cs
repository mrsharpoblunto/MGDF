using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;

namespace MGDF.GamesManager.Model.Factories
{
    /// <summary>
    /// This interface is used to allow factories to be registered as components in the factory service locator
    /// </summary>
    public interface IEntityFactory
    {
        IGameInstall CreateGameInstall(string installGame);
        IUpdate CreateUpdate(IGame game, IArchiveFile file);
        IPreferences CreatePreferences(string preferenceFile);
        IGame CreateGame(string filename);
    }

    public class EntityFactory: IEntityFactory
    {
        public static IEntityFactory Current
        {
            get
            {
                return ServiceLocator.Current.Get<IEntityFactory>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public IGameInstall CreateGameInstall(string installGame)
        {
            return new GameInstall(installGame);
        }

        public IUpdate CreateUpdate(IGame game, IArchiveFile file)
        {
            return new Update(game, file);
        }

        public IPreferences CreatePreferences(string preferenceFile)
        {
            return new Preferences(preferenceFile);
        }

        public IGame CreateGame(string filename)
        {
            return new Game(filename);
        }
    }
}
