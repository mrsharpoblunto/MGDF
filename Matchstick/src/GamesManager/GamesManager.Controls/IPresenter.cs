using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Controls
{
    public interface IPresenter
    {
        void ShowView();
        void ShowView(IView owner);
        void CloseView();
    }
}
