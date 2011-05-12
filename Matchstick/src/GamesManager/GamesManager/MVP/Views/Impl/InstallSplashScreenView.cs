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
    partial class InstallSplashScreenView : GamesManagerViewBase, ISplashScreenView
    {
        public InstallSplashScreenView()
        {
            InitializeComponent();
        }

        public event EventHandler OnInstall;

        public string Version
        {
            set { vesionLabel.Text = "Version " + value; }
        }

        public string GameName
        {
            set { TitleLabel.Text = value; }
        }

        public string Developer
        {
            set { DeveloperLabel.Text = value; }
        }

        public string Description
        {
            set { descriptionLabel.Text = value; }
        }

        public Image SplashImage
        {
            set
            {
                BackgroundImage = value;
                Invalidate();
            }
        }

        public Image GameIcon
        {
            set { GameIconPictureBox.Image = value; }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (OnInstall!=null)
            {
                OnInstall(this,new EventArgs());
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            CloseView();
        }
    }
}