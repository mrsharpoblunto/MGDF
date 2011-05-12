using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events
{
    public class AddGameEventArgs: EventArgs
    {
        public string GameFile { get; set; }
    }
}
