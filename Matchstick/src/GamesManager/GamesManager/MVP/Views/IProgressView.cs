using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views
{
    public interface IProgressView: IView
    {
        event EventHandler OnCancel;

        bool AllowCancel { set; }
        void Hide();
        void HideProgress();
        string Title { set; }
        string Details { set; }
        void ShowProgress(long progress, long total);
        Image GameIcon { set; }
    }
}