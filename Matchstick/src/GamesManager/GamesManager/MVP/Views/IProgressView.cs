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
        event EventHandler OnPause;
        event EventHandler OnResume;

        bool AllowPauseOrResume { set; }
        void Hide();
        void HideProgress();
        bool Paused { set; }
        string Title { set; }
        string Details { set; }
        void ShowProgress(long progress, long total);
        Image GameIcon { set; }
    }
}