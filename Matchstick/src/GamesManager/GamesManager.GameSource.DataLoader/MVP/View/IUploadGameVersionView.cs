using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View
{
    public interface IUploadGameVersionView:IView
    {
        event EventHandler OnCancel;

        int Progress { set; }
    }
}
