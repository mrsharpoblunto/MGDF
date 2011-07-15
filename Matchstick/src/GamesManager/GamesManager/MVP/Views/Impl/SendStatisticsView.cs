using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class SendStatisticsView : GamesManagerViewBase,ISendStatisticsView
    {
        private Game _game;

        public SendStatisticsView()
        {
            InitializeComponent();
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
        }

        public event EventHandler OnAllow;
        public event EventHandler OnDeny;

        public Game Game
        {
            set
            { 
                _game = value;
                Uri statsUrl = new Uri(_game.StatisticsService);
                label3.Text = _game.Name + " is requesting permission to send gameplay statistics to " + statsUrl.Host;
                label1.Text = _game.Name + " is requesting permission to send anonymous gameplay statistics to its developer "+_game.DeveloperName+@" in order to help improve this game.

Do you give your permission for these statistics to be sent?";
            }
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            try
            {
                Uri statisticsPrviacyPolicy = new Uri(_game.StatisticsPrivacyPolicy);
                Process.Start(statisticsPrviacyPolicy.ToString());
            }
            catch (Exception)
            {
                
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (OnAllow!=null)
            {
                OnAllow(this,new EventArgs());
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (OnDeny!=null)
            {
                OnDeny(this,new EventArgs());
            }
        }
    }
}