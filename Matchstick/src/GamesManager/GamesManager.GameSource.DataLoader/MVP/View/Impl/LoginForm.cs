using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class LoginForm : ViewBase, ILoginView 
    {
        public LoginForm()
        {
            InitializeComponent();
        }

        public event EventHandler OnLogin;

        public string DeveloperKey
        {
            get { return DeveloperKeyTextBox.Text; }
            set { DeveloperKeyTextBox.Text = value; }
        }

        public string SecretKey
        {
            get { return secretKeyTextBox.Text; }
            set { secretKeyTextBox.Text = value; }
        }

        public string GameSourceUrl
        {
            get { return GameSourceUrlTextBox.Text; }
            set { GameSourceUrlTextBox.Text = value; }
        }

        public bool RememberMe
        {
            get { return RememberMeCheckBox.Checked; }
            set { RememberMeCheckBox.Checked = value; }
        }

        public void ShowError(string error)
        {
            ErrorLabel.Visible = true;
            ErrorLabel.Text = error;
        }

        public bool ShowLoggingIn
        {
            set { panel1.Visible = !value;}
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            CloseView();
        }

        private void OKButton_Click(object sender, EventArgs e)
        {
            if (OnLogin!=null)
            {
                OnLogin(this,new EventArgs());
            }
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }
    }
}