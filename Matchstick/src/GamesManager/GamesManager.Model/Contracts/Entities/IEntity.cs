using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IEntity
    {
        List<string> ErrorCollection { get; }
        bool IsValid { get; }
    }
}