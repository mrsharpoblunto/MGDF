using System;
using System.Collections.Generic;
using System.Text;

namespace MGDF.GamesManager.Model.Exceptions
{
    public class GameException: Exception
    {
        internal GameException(string message): base(message)
        {
        }
    }
}
