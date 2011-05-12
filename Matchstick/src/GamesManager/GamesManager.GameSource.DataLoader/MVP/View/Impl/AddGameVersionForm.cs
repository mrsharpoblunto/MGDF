using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Events;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class AddGameVersionForm : ViewBase, IAddGameVersionView
    {
        public event EventHandler OnAddGame;

        public AddGameVersionForm()
        {
            InitializeComponent();
        }

        protected override void ShowView(IWin32Window owner)
        {
            ShowDialog(owner);
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
        public byte[] GameIconData
        {
            set
            {
                if (value == null)
                {
                    thumbnailImageBox.Image = null;
                }
                else
                {
                    using (Stream ms = new MemoryStream(value))
                    {
                        thumbnailImageBox.Image = Image.FromStream(ms);
                    }
                }
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public bool RequiresAuthentication
        {
            set { requireAuthenticationCheckbox.Checked = value; }
            get { return requireAuthenticationCheckbox.Checked; }
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
        public bool Published
        {
            get { return publishCheckBox.Checked; }
            set { publishCheckBox.Checked = value; }
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

        private void OKButton_Click(object sender, EventArgs e)
        {
            Cursor = Cursors.WaitCursor;
            Application.DoEvents();
            if (OnAddGame!=null)
            {
                OnAddGame(this,new EventArgs());
            }
            Cursor = Cursors.Default;
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            CloseView();
        }
    }
}
