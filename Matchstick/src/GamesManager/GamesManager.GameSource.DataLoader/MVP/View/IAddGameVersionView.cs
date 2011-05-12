using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View
{
    public interface IAddGameVersionViewBase : IView
    {
        string Uid { set; }
        string GameName { set; }
        int InterfaceVersion { set; }
        string Description { set; }
        string Homepage { set; }
        bool RequiresAuthentication { get; set; }
        Version Version { set; }
        bool Published { get; set; }

        void UpdateVersions(bool isUpdate, string UpdateMinVersion, string UpdateMaxVersion);
    }

    public interface IAddGameVersionView : IAddGameVersionViewBase
    {
        event EventHandler OnAddGame;
        byte[] GameIconData { set; }
    }
}
