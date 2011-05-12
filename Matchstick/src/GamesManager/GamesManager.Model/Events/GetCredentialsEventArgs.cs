using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model.Events
{
    public class GetCredentialsEventArgs: EventArgs
    {
        public string UserName { get; set; }
        public string Password { get; set;}
    }
}
