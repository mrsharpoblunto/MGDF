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
        private bool _allowCancel=true;

        public ProgressView()
        {
            InitializeComponent();
            HideProgress();
            Closing += ProgressView_OnClosing;
        }

        void ProgressView_OnClosing(object sender, CancelEventArgs e)
        {
            if (_userClosed && OnCancel!=null)
            {
                if (_allowCancel)
                {
                    OnCancel(this, new EventArgs()); 
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

        public event EventHandler OnCancel;

        public bool AllowCancel
        {
            set
            {
                _allowCancel = value;
                cancelButton.Visible = value;
                Progress.Width = value ? (cancelButton.Left - Progress.Left - DpiScaleX(8)) : (cancelButton.Right - Progress.Left);
            }
        }


        public void HideProgress()
        {
            ClientSize = new Size(ClientSize.Width, DpiScaleY(64));
            Progress.Visible = false;
            cancelButton.Visible = false;
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
                cancelButton.Visible = _allowCancel;
            }
            Progress.Value = total == 0 ? 0 : (int)((progress/(double)total)*100);
        }

        public Image GameIcon
        {
            set { GameIconPictureBox.Image = value; }
        }

        private void PauseOrResumeButton_Click(object sender, EventArgs e)
        {
            if (OnCancel!=null)
            {
                OnCancel(this, new EventArgs());
            }
        }
    }
}