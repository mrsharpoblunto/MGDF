using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class ProgressView : GamesManagerViewBase,IProgressView
    {
        private bool _userClosed=true;
        private bool _allowPauseOrResume=true;

        public ProgressView()
        {
            InitializeComponent();
            HideProgress();
            Closing += ProgressView_OnClosing;
        }

        void ProgressView_OnClosing(object sender, CancelEventArgs e)
        {
            if (_userClosed && OnPause!=null)
            {
                if (_allowPauseOrResume)
                {
                    OnPause(this,new EventArgs()); 
                }
                else
                {
                    e.Cancel = true;//can't close if we can't pause/resume this operation.
                }
            }
        }

        public override void CloseView()
        {
            _userClosed = false;
            base.CloseView();
        }

        public event EventHandler OnPause;
        public event EventHandler OnResume;

        public bool AllowPauseOrResume
        {
            set
            {
                _allowPauseOrResume = value;
                PauseOrResumeButton.Visible = value;
                Progress.Width = value ? (PauseOrResumeButton.Left - Progress.Left - DpiScaleX(8)) : (PauseOrResumeButton.Right - Progress.Left);
            }
        }


        public void HideProgress()
        {
            ClientSize = new Size(ClientSize.Width, DpiScaleY(64));
            Progress.Visible = false;
            PauseOrResumeButton.Visible = false;
        }

        public bool Paused
        {
            set { PauseOrResumeButton.Text = value ? "Resume" : "Pause"; }
        }

        public string Title
        {
            set
            {
                Text = value;
                TitleLabel.Text = value;
            }
        }

        public string Details
        {
            set { DetailsLabel.Text = value; }
        }

        public void ShowProgress(long progress, long total)
        {
            if (!Progress.Visible)
            {
                ClientSize = new Size(ClientSize.Width,DpiScaleY(128));
                Progress.Visible = true;
                PauseOrResumeButton.Visible = _allowPauseOrResume;
            }
            Progress.Value = (int)((progress/(double)total)*100);
        }

        public Image GameIcon
        {
            set { GameIconPictureBox.Image = value; }
        }

        private void PauseOrResumeButton_Click(object sender, EventArgs e)
        {
            if (PauseOrResumeButton.Text =="Resume")
            {
                if (OnResume!=null)
                {
                    OnResume(this,new EventArgs());
                }
            }
            else
            {
                if (OnPause != null)
                {
                    OnPause(this, new EventArgs());
                }
            }
        }
    }
}