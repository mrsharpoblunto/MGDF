using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class GetUpdatePermissionView : GamesManagerViewBase, IGetUpdatePermissionView
    {
        public event EventHandler OnUpdate;

        public Game Game
        {
            set { label1.Text = "An update for " + value.Name + " is available, would you like to download and install it now?"; }
        }

        public GetUpdatePermissionView()
        {
            InitializeComponent();
            AcceptButton = updateButton;
            CancelButton = cancelButton;
            Shown += GetUpgradePermissionView_Shown;
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }

        void GetUpgradePermissionView_Shown(object sender, EventArgs e)
        {
            if (UACControl.IsVistaOrHigher() && !UACControl.IsAdmin())
            {
                UACControl.AddShieldToButton(updateButton);
            }
        }

        private void updateButton_Click(object sender, EventArgs e)
        {
            if (OnUpdate!=null)
            {
                OnUpdate(this,new EventArgs());
            }
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            CloseView();
        }
    }
}
