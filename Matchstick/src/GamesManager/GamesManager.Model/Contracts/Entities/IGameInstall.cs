using System;
using System.Collections.Generic;
using System.Text;
using MGDF.GamesManager.Model.Contracts;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IGameInstall : IEntity
    {
        /// <summary>
        /// Information on the Game to install/upgrade
        /// </summary>
        IGame Game { get; }

        /// <summary>
        /// Information on what Game this installer upgrades (if any)
        /// </summary>
        IUpdate Update { get; }

        bool IsUpdate { get; }

        IArchiveFile GameContents { get; }

        string InstallerFile { get; }
    }
}