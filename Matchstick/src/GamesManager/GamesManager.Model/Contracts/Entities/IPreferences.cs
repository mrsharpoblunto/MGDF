using System;
using System.Collections.Generic;
using System.Text;
using MGDF.GamesManager.Model.Contracts;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    public interface IPreferences : IEntity
    {
        List<IPreference> PreferenceCollection { get; }
        void Save(string filename);
        void LoadAdditionalPreferences(string filename);
    }
}