using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IPreference
    {
        string Name { get; }
        string Value { get; set; }
    }
}