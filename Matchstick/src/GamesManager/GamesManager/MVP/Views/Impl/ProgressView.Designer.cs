namespace MGDF.GamesManager.MVP.Views.Impl
{
    partial class ProgressView
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
            this.TitleLabel = new System.Windows.Forms.Label();
            this.DetailsLabel = new System.Windows.Forms.Label();
            this.Progress = new System.Windows.Forms.ProgressBar();
            this.PauseOrResumeButton = new System.Windows.Forms.Button();
            this.GameIconPictureBox = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.GameIconPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // TitleLabel
            // 
            this.TitleLabel.AutoEllipsis = true;
            this.TitleLabel.BackColor = System.Drawing.Color.Transparent;
            this.TitleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TitleLabel.ForeColor = System.Drawing.Color.White;
            this.TitleLabel.Location = new System.Drawing.Point(62, 16);
            this.TitleLabel.Name = "TitleLabel";
            this.TitleLabel.Size = new System.Drawing.Size(246, 20);
            this.TitleLabel.TabIndex = 0;
            this.TitleLabel.Text = "Updating ##################";
            // 
            // DetailsLabel
            // 
            this.DetailsLabel.AutoEllipsis = true;
            this.DetailsLabel.BackColor = System.Drawing.Color.Transparent;
            this.DetailsLabel.ForeColor = System.Drawing.Color.White;
            this.DetailsLabel.Location = new System.Drawing.Point(62, 36);
            this.DetailsLabel.Name = "DetailsLabel";
            this.DetailsLabel.Size = new System.Drawing.Size(245, 13);
            this.DetailsLabel.TabIndex = 7;
            this.DetailsLabel.Text = "Installing update (1/5)";
            // 
            // Progress
            // 
            this.Progress.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                                                                         | System.Windows.Forms.AnchorStyles.Right)));
            this.Progress.Location = new System.Drawing.Point(16, 82);
            this.Progress.Name = "Progress";
            this.Progress.Size = new System.Drawing.Size(207, 23);
            this.Progress.TabIndex = 8;
            // 
            // PauseOrResumeButton
            // 
            this.PauseOrResumeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.PauseOrResumeButton.Location = new System.Drawing.Point(229, 82);
            this.PauseOrResumeButton.Name = "PauseOrResumeButton";
            this.PauseOrResumeButton.Size = new System.Drawing.Size(75, 23);
            this.PauseOrResumeButton.TabIndex = 9;
            this.PauseOrResumeButton.Text = "Pause";
            this.PauseOrResumeButton.UseVisualStyleBackColor = true;
            this.PauseOrResumeButton.Click += new System.EventHandler(this.PauseOrResumeButton_Click);
            // 
            // GameIconPictureBox
            // 
            this.GameIconPictureBox.BackColor = System.Drawing.Color.Black;
            this.GameIconPictureBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.GameIconPictureBox.Image = global::MGDF.GamesManager.Properties.Resources.gameIcon;
            this.GameIconPictureBox.Location = new System.Drawing.Point(8, 8);
            this.GameIconPictureBox.Name = "GameIconPictureBox";
            this.GameIconPictureBox.Size = new System.Drawing.Size(48, 48);
            this.GameIconPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.GameIconPictureBox.TabIndex = 1;
            this.GameIconPictureBox.TabStop = false;
            // 
            // ProgressView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(319, 123);
            this.Controls.Add(this.GameIconPictureBox);
            this.Controls.Add(this.PauseOrResumeButton);
            this.Controls.Add(this.DetailsLabel);
            this.Controls.Add(this.Progress);
            this.Controls.Add(this.TitleLabel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Location = new System.Drawing.Point(0, 0);
            this.MaximizeBox = false;
            this.Name = "ProgressView";
            this.ShowIcon = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Updating ##############";
            ((System.ComponentModel.ISupportInitialize)(this.GameIconPictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PictureBox GameIconPictureBox;
        private System.Windows.Forms.Label TitleLabel;
        private System.Windows.Forms.Label DetailsLabel;
        private System.Windows.Forms.ProgressBar Progress;
        private System.Windows.Forms.Button PauseOrResumeButton;
    }
}