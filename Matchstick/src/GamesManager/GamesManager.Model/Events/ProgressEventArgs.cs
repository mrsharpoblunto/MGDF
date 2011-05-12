using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model.Events
{
    public class ProgressEventArgs: EventArgs
    {
        public uint Progress { get; set; }
        public uint Total { get; set; }
    }
}
