using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Contracts.Entities;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class GetCredentialsView : GamesManagerViewBase, IGetCredentialsView
    {
        public GetCredentialsView()
        {
            InitializeComponent();
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }

        public event EventHandler OnOK;

        public IGame Game
        {
            set
            {
                Uri gameSourceUri = new Uri(value.GameSourceService);
                label1.Text = "Please enter your credentials to download " + value.Name + " from " + gameSourceUri.Host;
            }
        }

        public string Password
        {
            get { return maskedTextBox1.Text; }
        }

        public string Username
        {
            get { return textBox1.Text; }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            CloseView();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (OnOK!=null)
            {
                OnOK(this,new EventArgs());
            }
        }
    }
}