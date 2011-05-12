using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model.Events
{
    public class GetStatsPermissionEventArgs: EventArgs
    {
        public string Url { get; set;}
        public string PrivacyPolicyUrl { get; set; }
    }
}
