using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
    public interface ISubmitErrorView : IView
    {
        event EventHandler CopyLogOutput;
        event EventHandler EmailLogOutput;

        string Message { set; }
        string SupportEmail { set; }
    }
}