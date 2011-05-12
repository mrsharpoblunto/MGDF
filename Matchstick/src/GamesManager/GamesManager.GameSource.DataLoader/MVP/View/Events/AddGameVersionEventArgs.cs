using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.GameSource.Contracts.Entities;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events
{
    public class AddGameVersionEventArgs : AddGameEventArgs
    {
        public Game Game { get; set; }
    }
}
