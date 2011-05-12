using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class UploadGameVersionForm : ViewBase,IUploadGameVersionView
    {
        public event EventHandler OnCancel;
        private bool _forceClose = false;

        public UploadGameVersionForm()
        {
            InitializeComponent();
            Closing += UploadGameForm_Closing;
        }

        void UploadGameForm_Closing(object sender, CancelEventArgs e)
        {
            e.Cancel = !_forceClose;
        }

        public override void CloseView()
        {
            _forceClose = true;
            Close();
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }

        public int Progress
        {
            set { progressBar1.Value = value; }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (OnCancel!=null)
            {
                OnCancel(this,new EventArgs());
            }
        }
    }
}
