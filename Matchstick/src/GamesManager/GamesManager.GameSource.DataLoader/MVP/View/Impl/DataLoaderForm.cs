using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class DataLoaderForm : ViewBase, IDataLoaderView 
    {
        public DataLoaderForm()
        {
            InitializeComponent();
        }

        public event EventHandler OnLogout;
        public event EventHandler OnRefresh;
        public event EventHandler<SelectGameEventArgs> OnSelectGame;
        public event EventHandler<SelectGameEventArgs> OnDeleteGame;
        public event EventHandler<SelectGameEventArgs> OnUpdateGame;
        public event EventHandler<AddGameEventArgs> OnAddGame;
        public event EventHandler<AddGameVersionEventArgs> OnAddGameVersion;
        public event EventHandler<SelectGameVersionEventArgs> OnSelectGameVersion;
        public event EventHandler<SelectGameVersionEventArgs> OnDeleteGameVersion;
        public event EventHandler<SelectGameVersionEventArgs> OnUpdateGameVersion;

        public List<Game> Games
        {
            set
            {
                GamesListBox.DisplayMember = "Name";
                GamesListBox.ValueMember = "Uid";
                GamesListBox.DataSource = value;

                if (value.Count>0 && OnSelectGame != null)
                {
                    OnSelectGame(this, new SelectGameEventArgs { Game = value[0] });
                }
            }
        }

        public List<GameVersion> GameVersions
        {
            set
            {
                GameVersionsListBox.DataSource = value;

                UpdateGameVersionButton.Enabled = value.Count > 0;
                RemoveGameVersionButton.Enabled = value.Count > 0;

                if (value.Count > 0 && OnSelectGameVersion != null)
                {
                    OnSelectGameVersion(this, new SelectGameVersionEventArgs { GameVersion = value[0] });
                }
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string GameName
        {
            set { NameTextBox.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string Description
        {
            set { DescriptionTextBox.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string Homepage
        {
            set { homepageLink.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string Uid
        {
            set { UidtextBox.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public int InterfaceVersion
        {
            set { interfaceVersionTextBox.Text = value.ToString(); }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Version Version
        {
            set { versionTextBox.Text = value.ToString(); }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string UpdateDescription
        {
            set { updateDescriptionTextBox.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string DownloadURL
        {
            set { downloadLink.Text = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool Published
        {
            get { return publishedCheckBox.Checked; }
            set { publishedCheckBox.Checked = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool RequiresAuthentication
        {
            get { return requiresAuthenticationCheckbox.Checked; }
            set { requiresAuthenticationCheckbox.Checked = value; }
        }

        public void UpdateVersions(bool isUpdate, string UpdateMinVersion, string UpdateMaxVersion)
        {
            if (isUpdate)
            {
                updateLabel.Visible = true;
                updateLabel.Text = "(" + UpdateMinVersion + " - " + UpdateMaxVersion + ")";
            }
            else
            {
                updateLabel.Visible = false;
            }
        }

        public void ShowSelectedGame(bool visible)
        {
            gameContainer.Visible = visible;
        }

        public void ShowSelectedGameVersion(bool visible)
        {
            gameVersionContainer.Visible = visible;
        }

        private void logoutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (OnLogout!=null)
            {
                OnLogout(this,new EventArgs());
            }
        }

        private void GamesListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (OnSelectGame!=null && GamesListBox.SelectedItem!=null)
            {
                Cursor = Cursors.WaitCursor;
                Application.DoEvents();
                OnSelectGame(this, new SelectGameEventArgs { Game = GamesListBox.SelectedItem as Game });
                Cursor = Cursors.Default;
            }
        }

        private void DeleteButton_Click(object sender, EventArgs e)
        {
            if (ViewFactory.Current.CreateView<IMessage>().Show("Do you really want to delete this Game?", "Delete Game", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.Yes)
            {
                if (OnDeleteGame != null && GamesListBox.SelectedItem != null)
                {
                    Cursor = Cursors.WaitCursor;
                    Application.DoEvents();
                    OnDeleteGame(this, new SelectGameEventArgs { Game = GamesListBox.SelectedItem as Game });
                    Cursor = Cursors.Default;
                }
            }
        }

        private void UpdateButton_Click(object sender, EventArgs e)
        {
            if (OnUpdateGame != null && GamesListBox.SelectedItem != null)
            {
                Cursor = Cursors.WaitCursor;
                Application.DoEvents();
                OnUpdateGame(this, new SelectGameEventArgs { Game = GamesListBox.SelectedItem as Game });
                Cursor = Cursors.Default;
            }
        }

        private void AddGameVersionButton_Click(object sender, EventArgs e)
        {
            if (OnAddGameVersion != null && GamesListBox.SelectedItem != null && openFileDialog1.ShowDialog(this) == DialogResult.OK)
            {
                OnAddGameVersion(this, new AddGameVersionEventArgs { GameFile = openFileDialog1.FileName, Game = GamesListBox.SelectedItem as Game});
            }
        }


        private void AddGameButton_Click(object sender, EventArgs e)
        {
            if (OnAddGame != null && openFileDialog1.ShowDialog(this) == DialogResult.OK)
            {
                OnAddGame(this, new AddGameEventArgs { GameFile = openFileDialog1.FileName });
            }
        }

        private void GameVersionsListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (OnSelectGameVersion != null && GameVersionsListBox.SelectedItem != null)
            {
                Cursor = Cursors.WaitCursor;
                Application.DoEvents();
                OnSelectGameVersion(this, new SelectGameVersionEventArgs { GameVersion = GameVersionsListBox.SelectedItem as GameVersion, Game = GamesListBox.SelectedItem as Game });
                Cursor = Cursors.Default;
            }
        }

        private void RemoveGameVersionButton_Click(object sender, EventArgs e)
        {
            if (ViewFactory.Current.CreateView<IMessage>().Show("Do you really want to delete this Game version?", "Delete Game version", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.Yes)
            {
                if (OnDeleteGameVersion != null && GameVersionsListBox.SelectedItem != null)
                {
                    Cursor = Cursors.WaitCursor;
                    Application.DoEvents();
                    OnDeleteGameVersion(this, new SelectGameVersionEventArgs { GameVersion = GameVersionsListBox.SelectedItem as GameVersion, Game = GamesListBox.SelectedItem as Game });
                    Cursor = Cursors.Default;
                }
            }
        }

        private void UpdateGameVersionButton_Click(object sender, EventArgs e)
        {
            if (OnUpdateGameVersion != null && GameVersionsListBox.SelectedItem != null)
            {
                Cursor = Cursors.WaitCursor;
                Application.DoEvents();
                OnUpdateGameVersion(this, new SelectGameVersionEventArgs { GameVersion = GameVersionsListBox.SelectedItem as GameVersion, Game = GamesListBox.SelectedItem as Game });
                Cursor = Cursors.Default;
            }
        }

        private void downloadLink_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start(downloadLink.Text);
        }

        private void homepageLink_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start(homepageLink.Text);
        }

        private void refreshToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (OnRefresh!=null)
            {
                OnRefresh(this,new EventArgs());
            }
        }

        private void requiresAuthenticationCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (GamesListBox.SelectedItem != null)
            {
                (GamesListBox.SelectedItem as Game).RequiresAuthentication = RequiresAuthentication;
            }
        }

        private void publishedCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (GameVersionsListBox.SelectedItem != null)
            {
                (GameVersionsListBox.SelectedItem as GameVersion).Published = Published;
            }
        }
    }
}