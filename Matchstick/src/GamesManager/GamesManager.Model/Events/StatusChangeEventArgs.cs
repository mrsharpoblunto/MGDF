using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.Model.Events
{
    public class StatusChangeEventArgs: EventArgs
    {
        public PendingOperationStatus Status { get; set; }
    }
}
