using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model.Events
{
    public class ReloadGameEventArgs: EventArgs
    {
        public bool Abort { get; set; }
    }
}
