using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class CheckUninstallView : GamesManagerViewBase,ICheckUninstallView
    {
        public CheckUninstallView()
        {
            InitializeComponent();
        }

        public event EventHandler OnUninstall;

        public List<string> DependantGames
        {
            set { listBox1.DataSource = value; }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (OnUninstall!=null)
            {
                OnUninstall(this,new EventArgs());
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            CloseView();
        }
    }
}