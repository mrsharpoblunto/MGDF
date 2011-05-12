using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class DataLoaderForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.GamesListBox = new System.Windows.Forms.ListBox();
            this.gameContainer = new System.Windows.Forms.GroupBox();
            this.updateGameButton = new System.Windows.Forms.Button();
            this.requiresAuthenticationCheckbox = new System.Windows.Forms.CheckBox();
            this.homepageLink = new System.Windows.Forms.LinkLabel();
            this.interfaceVersionTextBox = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.removeGameButton = new System.Windows.Forms.Button();
            this.UidtextBox = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.DescriptionTextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.NameTextBox = new System.Windows.Forms.TextBox();
            this.gameVersionPanel = new System.Windows.Forms.GroupBox();
            this.UpdateGameVersionButton = new System.Windows.Forms.Button();
            this.gameVersionContainer = new System.Windows.Forms.Panel();
            this.versionTextBox = new System.Windows.Forms.TextBox();
            this.publishedCheckBox = new System.Windows.Forms.CheckBox();
            this.downloadLink = new System.Windows.Forms.LinkLabel();
            this.label10 = new System.Windows.Forms.Label();
            this.updateLabel = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.updateDescriptionTextBox = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.RemoveGameVersionButton = new System.Windows.Forms.Button();
            this.AddGameVersionButton = new System.Windows.Forms.Button();
            this.GameVersionsListBox = new System.Windows.Forms.ListBox();
            this.button1 = new System.Windows.Forms.Button();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.logoutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.logoutToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.refreshToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.gameContainer.SuspendLayout();
            this.gameVersionPanel.SuspendLayout();
            this.gameVersionContainer.SuspendLayout();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // GamesListBox
            // 
            this.GamesListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.GamesListBox.FormattingEnabled = true;
            this.GamesListBox.IntegralHeight = false;
            this.GamesListBox.Location = new System.Drawing.Point(12, 27);
            this.GamesListBox.Name = "GamesListBox";
            this.GamesListBox.Size = new System.Drawing.Size(153, 492);
            this.GamesListBox.TabIndex = 0;
            this.GamesListBox.SelectedIndexChanged += new System.EventHandler(this.GamesListBox_SelectedIndexChanged);
            // 
            // gameContainer
            // 
            this.gameContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.gameContainer.Controls.Add(this.updateGameButton);
            this.gameContainer.Controls.Add(this.requiresAuthenticationCheckbox);
            this.gameContainer.Controls.Add(this.homepageLink);
            this.gameContainer.Controls.Add(this.interfaceVersionTextBox);
            this.gameContainer.Controls.Add(this.label7);
            this.gameContainer.Controls.Add(this.removeGameButton);
            this.gameContainer.Controls.Add(this.UidtextBox);
            this.gameContainer.Controls.Add(this.label4);
            this.gameContainer.Controls.Add(this.label3);
            this.gameContainer.Controls.Add(this.DescriptionTextBox);
            this.gameContainer.Controls.Add(this.label2);
            this.gameContainer.Controls.Add(this.label1);
            this.gameContainer.Controls.Add(this.NameTextBox);
            this.gameContainer.Controls.Add(this.gameVersionPanel);
            this.gameContainer.Location = new System.Drawing.Point(171, 27);
            this.gameContainer.Name = "gameContainer";
            this.gameContainer.Size = new System.Drawing.Size(561, 541);
            this.gameContainer.TabIndex = 1;
            this.gameContainer.TabStop = false;
            this.gameContainer.Text = "Game Information";
            this.gameContainer.Visible = false;
            // 
            // updateGameButton
            // 
            this.updateGameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.updateGameButton.Location = new System.Drawing.Point(435, 512);
            this.updateGameButton.Name = "updateGameButton";
            this.updateGameButton.Size = new System.Drawing.Size(103, 23);
            this.updateGameButton.TabIndex = 5;
            this.updateGameButton.Text = "Update Game";
            this.updateGameButton.UseVisualStyleBackColor = true;
            this.updateGameButton.Click += new System.EventHandler(this.UpdateButton_Click);
            // 
            // requiresAuthenticationCheckbox
            // 
            this.requiresAuthenticationCheckbox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.requiresAuthenticationCheckbox.AutoSize = true;
            this.requiresAuthenticationCheckbox.Location = new System.Drawing.Point(405, 172);
            this.requiresAuthenticationCheckbox.Name = "requiresAuthenticationCheckbox";
            this.requiresAuthenticationCheckbox.Size = new System.Drawing.Size(134, 17);
            this.requiresAuthenticationCheckbox.TabIndex = 49;
            this.requiresAuthenticationCheckbox.Text = "Require Authentication";
            this.requiresAuthenticationCheckbox.UseVisualStyleBackColor = true;
            this.requiresAuthenticationCheckbox.CheckedChanged += new System.EventHandler(this.requiresAuthenticationCheckbox_CheckedChanged);
            // 
            // homepageLink
            // 
            this.homepageLink.AutoSize = true;
            this.homepageLink.Location = new System.Drawing.Point(103, 146);
            this.homepageLink.Name = "homepageLink";
            this.homepageLink.Size = new System.Drawing.Size(55, 13);
            this.homepageLink.TabIndex = 48;
            this.homepageLink.TabStop = true;
            this.homepageLink.Text = "linkLabel1";
            this.homepageLink.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.homepageLink_LinkClicked);
            // 
            // interfaceVersionTextBox
            // 
            this.interfaceVersionTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.interfaceVersionTextBox.Location = new System.Drawing.Point(105, 169);
            this.interfaceVersionTextBox.Name = "interfaceVersionTextBox";
            this.interfaceVersionTextBox.ReadOnly = true;
            this.interfaceVersionTextBox.Size = new System.Drawing.Size(86, 20);
            this.interfaceVersionTextBox.TabIndex = 47;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(12, 172);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(87, 13);
            this.label7.TabIndex = 46;
            this.label7.Text = "Interface Version";
            // 
            // removeGameButton
            // 
            this.removeGameButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.removeGameButton.Location = new System.Drawing.Point(328, 512);
            this.removeGameButton.Name = "removeGameButton";
            this.removeGameButton.Size = new System.Drawing.Size(101, 23);
            this.removeGameButton.TabIndex = 3;
            this.removeGameButton.Text = "Remove Game";
            this.removeGameButton.UseVisualStyleBackColor = true;
            this.removeGameButton.Click += new System.EventHandler(this.DeleteButton_Click);
            // 
            // UidtextBox
            // 
            this.UidtextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.UidtextBox.Location = new System.Drawing.Point(106, 19);
            this.UidtextBox.Name = "UidtextBox";
            this.UidtextBox.ReadOnly = true;
            this.UidtextBox.Size = new System.Drawing.Size(439, 20);
            this.UidtextBox.TabIndex = 44;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(77, 22);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(23, 13);
            this.label4.TabIndex = 43;
            this.label4.Text = "Uid";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(40, 146);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(59, 13);
            this.label3.TabIndex = 41;
            this.label3.Text = "Homepage";
            // 
            // DescriptionTextBox
            // 
            this.DescriptionTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.DescriptionTextBox.Location = new System.Drawing.Point(106, 71);
            this.DescriptionTextBox.Multiline = true;
            this.DescriptionTextBox.Name = "DescriptionTextBox";
            this.DescriptionTextBox.ReadOnly = true;
            this.DescriptionTextBox.Size = new System.Drawing.Size(440, 66);
            this.DescriptionTextBox.TabIndex = 40;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(40, 74);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 39;
            this.label2.Text = "Description";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(65, 48);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(35, 13);
            this.label1.TabIndex = 38;
            this.label1.Text = "Name";
            // 
            // NameTextBox
            // 
            this.NameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.NameTextBox.Location = new System.Drawing.Point(106, 45);
            this.NameTextBox.Name = "NameTextBox";
            this.NameTextBox.ReadOnly = true;
            this.NameTextBox.Size = new System.Drawing.Size(439, 20);
            this.NameTextBox.TabIndex = 37;
            // 
            // gameVersionPanel
            // 
            this.gameVersionPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.gameVersionPanel.Controls.Add(this.UpdateGameVersionButton);
            this.gameVersionPanel.Controls.Add(this.gameVersionContainer);
            this.gameVersionPanel.Controls.Add(this.RemoveGameVersionButton);
            this.gameVersionPanel.Controls.Add(this.AddGameVersionButton);
            this.gameVersionPanel.Controls.Add(this.GameVersionsListBox);
            this.gameVersionPanel.Location = new System.Drawing.Point(15, 221);
            this.gameVersionPanel.Name = "gameVersionPanel";
            this.gameVersionPanel.Size = new System.Drawing.Size(530, 271);
            this.gameVersionPanel.TabIndex = 1;
            this.gameVersionPanel.TabStop = false;
            this.gameVersionPanel.Text = "Game Versions";
            // 
            // UpdateGameVersionButton
            // 
            this.UpdateGameVersionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.UpdateGameVersionButton.Location = new System.Drawing.Point(420, 242);
            this.UpdateGameVersionButton.Name = "UpdateGameVersionButton";
            this.UpdateGameVersionButton.Size = new System.Drawing.Size(103, 23);
            this.UpdateGameVersionButton.TabIndex = 6;
            this.UpdateGameVersionButton.Text = "Update Version";
            this.UpdateGameVersionButton.UseVisualStyleBackColor = true;
            this.UpdateGameVersionButton.Click += new System.EventHandler(this.UpdateGameVersionButton_Click);
            // 
            // gameVersionContainer
            // 
            this.gameVersionContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.gameVersionContainer.Controls.Add(this.versionTextBox);
            this.gameVersionContainer.Controls.Add(this.publishedCheckBox);
            this.gameVersionContainer.Controls.Add(this.downloadLink);
            this.gameVersionContainer.Controls.Add(this.label10);
            this.gameVersionContainer.Controls.Add(this.updateLabel);
            this.gameVersionContainer.Controls.Add(this.label5);
            this.gameVersionContainer.Controls.Add(this.updateDescriptionTextBox);
            this.gameVersionContainer.Controls.Add(this.label8);
            this.gameVersionContainer.Location = new System.Drawing.Point(151, 19);
            this.gameVersionContainer.Name = "gameVersionContainer";
            this.gameVersionContainer.Size = new System.Drawing.Size(372, 217);
            this.gameVersionContainer.TabIndex = 8;
            this.gameVersionContainer.Visible = false;
            // 
            // versionTextBox
            // 
            this.versionTextBox.Location = new System.Drawing.Point(67, 0);
            this.versionTextBox.Name = "versionTextBox";
            this.versionTextBox.ReadOnly = true;
            this.versionTextBox.Size = new System.Drawing.Size(86, 20);
            this.versionTextBox.TabIndex = 52;
            // 
            // publishedCheckBox
            // 
            this.publishedCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.publishedCheckBox.AutoSize = true;
            this.publishedCheckBox.Location = new System.Drawing.Point(8, 197);
            this.publishedCheckBox.Name = "publishedCheckBox";
            this.publishedCheckBox.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this.publishedCheckBox.Size = new System.Drawing.Size(72, 17);
            this.publishedCheckBox.TabIndex = 43;
            this.publishedCheckBox.Text = "Published";
            this.publishedCheckBox.UseVisualStyleBackColor = true;
            this.publishedCheckBox.CheckedChanged += new System.EventHandler(this.publishedCheckBox_CheckedChanged);
            // 
            // downloadLink
            // 
            this.downloadLink.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.downloadLink.AutoSize = true;
            this.downloadLink.Location = new System.Drawing.Point(67, 172);
            this.downloadLink.Name = "downloadLink";
            this.downloadLink.Size = new System.Drawing.Size(55, 13);
            this.downloadLink.TabIndex = 42;
            this.downloadLink.TabStop = true;
            this.downloadLink.Text = "linkLabel1";
            this.downloadLink.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.downloadLink_LinkClicked);
            // 
            // label10
            // 
            this.label10.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(41, 172);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(20, 13);
            this.label10.TabIndex = 41;
            this.label10.Text = "Url";
            // 
            // updateLabel
            // 
            this.updateLabel.AutoSize = true;
            this.updateLabel.Location = new System.Drawing.Point(157, 3);
            this.updateLabel.Name = "updateLabel";
            this.updateLabel.Size = new System.Drawing.Size(68, 13);
            this.updateLabel.TabIndex = 40;
            this.updateLabel.Text = "UpdateLabel";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(19, 3);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(42, 13);
            this.label5.TabIndex = 38;
            this.label5.Text = "Version";
            // 
            // updateDescriptionTextBox
            // 
            this.updateDescriptionTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.updateDescriptionTextBox.Location = new System.Drawing.Point(67, 26);
            this.updateDescriptionTextBox.Multiline = true;
            this.updateDescriptionTextBox.Name = "updateDescriptionTextBox";
            this.updateDescriptionTextBox.ReadOnly = true;
            this.updateDescriptionTextBox.Size = new System.Drawing.Size(306, 137);
            this.updateDescriptionTextBox.TabIndex = 37;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(1, 29);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(60, 13);
            this.label8.TabIndex = 36;
            this.label8.Text = "Description";
            // 
            // RemoveGameVersionButton
            // 
            this.RemoveGameVersionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.RemoveGameVersionButton.Location = new System.Drawing.Point(313, 242);
            this.RemoveGameVersionButton.Name = "RemoveGameVersionButton";
            this.RemoveGameVersionButton.Size = new System.Drawing.Size(101, 23);
            this.RemoveGameVersionButton.TabIndex = 5;
            this.RemoveGameVersionButton.Text = "Remove Version";
            this.RemoveGameVersionButton.UseVisualStyleBackColor = true;
            this.RemoveGameVersionButton.Click += new System.EventHandler(this.RemoveGameVersionButton_Click);
            // 
            // AddGameVersionButton
            // 
            this.AddGameVersionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.AddGameVersionButton.Location = new System.Drawing.Point(6, 242);
            this.AddGameVersionButton.Name = "AddGameVersionButton";
            this.AddGameVersionButton.Size = new System.Drawing.Size(139, 23);
            this.AddGameVersionButton.TabIndex = 4;
            this.AddGameVersionButton.Text = "Add New Version";
            this.AddGameVersionButton.UseVisualStyleBackColor = true;
            this.AddGameVersionButton.Click += new System.EventHandler(this.AddGameVersionButton_Click);
            // 
            // GameVersionsListBox
            // 
            this.GameVersionsListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.GameVersionsListBox.FormattingEnabled = true;
            this.GameVersionsListBox.IntegralHeight = false;
            this.GameVersionsListBox.Location = new System.Drawing.Point(7, 19);
            this.GameVersionsListBox.Name = "GameVersionsListBox";
            this.GameVersionsListBox.Size = new System.Drawing.Size(138, 217);
            this.GameVersionsListBox.TabIndex = 1;
            this.GameVersionsListBox.SelectedIndexChanged += new System.EventHandler(this.GameVersionsListBox_SelectedIndexChanged);
            // 
            // button1
            // 
            this.button1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button1.Location = new System.Drawing.Point(12, 539);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(153, 23);
            this.button1.TabIndex = 2;
            this.button1.Text = "Add New Game";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.AddGameButton_Click);
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.logoutToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(744, 24);
            this.menuStrip1.TabIndex = 4;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // logoutToolStripMenuItem
            // 
            this.logoutToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.logoutToolStripMenuItem1,
            this.refreshToolStripMenuItem});
            this.logoutToolStripMenuItem.Name = "logoutToolStripMenuItem";
            this.logoutToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.logoutToolStripMenuItem.Text = "File";
            // 
            // logoutToolStripMenuItem1
            // 
            this.logoutToolStripMenuItem1.Name = "logoutToolStripMenuItem1";
            this.logoutToolStripMenuItem1.Size = new System.Drawing.Size(132, 22);
            this.logoutToolStripMenuItem1.Text = "Logout";
            this.logoutToolStripMenuItem1.Click += new System.EventHandler(this.logoutToolStripMenuItem_Click);
            // 
            // refreshToolStripMenuItem
            // 
            this.refreshToolStripMenuItem.Name = "refreshToolStripMenuItem";
            this.refreshToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F5;
            this.refreshToolStripMenuItem.Size = new System.Drawing.Size(132, 22);
            this.refreshToolStripMenuItem.Text = "Refresh";
            this.refreshToolStripMenuItem.Click += new System.EventHandler(this.refreshToolStripMenuItem_Click);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.DefaultExt = "mza";
            this.openFileDialog1.Filter = "MGDF Game files|*.mza";
            this.openFileDialog1.Title = "Select MGDF Game";
            // 
            // DataLoaderForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(744, 578);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.gameContainer);
            this.Controls.Add(this.GamesListBox);
            this.Controls.Add(this.menuStrip1);
            this.Location = new System.Drawing.Point(0, 0);
            this.MainMenuStrip = this.menuStrip1;
            this.MinimumSize = new System.Drawing.Size(760, 561);
            this.Name = "DataLoaderForm";
            this.Text = "MGDF game source data loader";
            this.gameContainer.ResumeLayout(false);
            this.gameContainer.PerformLayout();
            this.gameVersionPanel.ResumeLayout(false);
            this.gameVersionContainer.ResumeLayout(false);
            this.gameVersionContainer.PerformLayout();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListBox GamesListBox;
        private System.Windows.Forms.GroupBox gameContainer;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button removeGameButton;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem logoutToolStripMenuItem;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.GroupBox gameVersionPanel;
        private System.Windows.Forms.Button updateGameButton;
        private System.Windows.Forms.Button UpdateGameVersionButton;
        private System.Windows.Forms.Button RemoveGameVersionButton;
        private System.Windows.Forms.Button AddGameVersionButton;
        private System.Windows.Forms.ListBox GameVersionsListBox;
        private System.Windows.Forms.Panel gameVersionContainer;
        private System.Windows.Forms.CheckBox requiresAuthenticationCheckbox;
        private System.Windows.Forms.LinkLabel homepageLink;
        private System.Windows.Forms.TextBox interfaceVersionTextBox;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox UidtextBox;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox DescriptionTextBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox NameTextBox;
        private System.Windows.Forms.CheckBox publishedCheckBox;
        private System.Windows.Forms.LinkLabel downloadLink;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label updateLabel;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox updateDescriptionTextBox;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox versionTextBox;
        private System.Windows.Forms.ToolStripMenuItem logoutToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem refreshToolStripMenuItem;
    }
}