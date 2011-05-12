using System;
using System.Collections.Generic;
using System.Text;
using MGDF.GamesManager.Model.Contracts;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IUpdate: IEntity
    {
        Version UpdateMinVersion { get; }
        Version UpdateMaxVersion { get; }
        List<string> RemoveFiles { get; }
        string UpdateDescription { get; }
        IGame UpdateTarget { get; }
    }
}