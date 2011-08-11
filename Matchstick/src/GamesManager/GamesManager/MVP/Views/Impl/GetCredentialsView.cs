using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class GetCredentialsView : GamesManagerViewBase, IGetCredentialsView
    {
        public GetCredentialsView()
        {
            InitializeComponent();
            AcceptButton = button2;
            CancelButton = button1;
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }

        public event EventHandler OnOK;

        public Game Game
        {
            set
            {
                Uri gameSourceUri = new Uri(value.GameSourceService);
                label3.Text = "Please enter your credentials to download " + value.Name + " from " + gameSourceUri.Host;
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