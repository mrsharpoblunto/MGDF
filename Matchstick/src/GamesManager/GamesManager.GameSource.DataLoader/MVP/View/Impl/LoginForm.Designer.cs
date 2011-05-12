namespace MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl
{
    partial class LoginForm
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
            this.DeveloperKeyTextBox = new System.Windows.Forms.TextBox();
            this.GameSourceUrlTextBox = new System.Windows.Forms.TextBox();
            this.RememberMeCheckBox = new System.Windows.Forms.CheckBox();
            this.CancelLoginButton = new System.Windows.Forms.Button();
            this.OKButton = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.ErrorLabel = new System.Windows.Forms.Label();
            this.secretKeyTextBox = new System.Windows.Forms.TextBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label4 = new System.Windows.Forms.Label();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // DeveloperKeyTextBox
            // 
            this.DeveloperKeyTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.DeveloperKeyTextBox.Location = new System.Drawing.Point(105, 0);
            this.DeveloperKeyTextBox.Name = "DeveloperKeyTextBox";
            this.DeveloperKeyTextBox.Size = new System.Drawing.Size(269, 20);
            this.DeveloperKeyTextBox.TabIndex = 0;
            // 
            // GameSourceUrlTextBox
            // 
            this.GameSourceUrlTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.GameSourceUrlTextBox.Location = new System.Drawing.Point(105, 52);
            this.GameSourceUrlTextBox.Name = "GameSourceUrlTextBox";
            this.GameSourceUrlTextBox.Size = new System.Drawing.Size(269, 20);
            this.GameSourceUrlTextBox.TabIndex = 2;
            // 
            // RememberMeCheckBox
            // 
            this.RememberMeCheckBox.AutoSize = true;
            this.RememberMeCheckBox.Location = new System.Drawing.Point(105, 78);
            this.RememberMeCheckBox.Name = "RememberMeCheckBox";
            this.RememberMeCheckBox.Size = new System.Drawing.Size(94, 17);
            this.RememberMeCheckBox.TabIndex = 3;
            this.RememberMeCheckBox.Text = "Remember me";
            this.RememberMeCheckBox.UseVisualStyleBackColor = true;
            // 
            // CancelLoginButton
            // 
            this.CancelLoginButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.CancelLoginButton.Location = new System.Drawing.Point(299, 99);
            this.CancelLoginButton.Name = "CancelLoginButton";
            this.CancelLoginButton.Size = new System.Drawing.Size(75, 23);
            this.CancelLoginButton.TabIndex = 4;
            this.CancelLoginButton.Text = "Cancel";
            this.CancelLoginButton.UseVisualStyleBackColor = true;
            this.CancelLoginButton.Click += new System.EventHandler(this.CancelButton_Click);
            // 
            // OKButton
            // 
            this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.OKButton.Location = new System.Drawing.Point(218, 99);
            this.OKButton.Name = "OKButton";
            this.OKButton.Size = new System.Drawing.Size(75, 23);
            this.OKButton.TabIndex = 5;
            this.OKButton.Text = "OK";
            this.OKButton.UseVisualStyleBackColor = true;
            this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(22, 7);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(77, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Developer Key";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(40, 29);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(59, 13);
            this.label2.TabIndex = 7;
            this.label2.Text = "Secret Key";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(5, 55);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(94, 13);
            this.label3.TabIndex = 8;
            this.label3.Text = "GameSource URL";
            // 
            // ErrorLabel
            // 
            this.ErrorLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.ErrorLabel.AutoSize = true;
            this.ErrorLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ErrorLabel.ForeColor = System.Drawing.Color.Red;
            this.ErrorLabel.Location = new System.Drawing.Point(5, 104);
            this.ErrorLabel.Name = "ErrorLabel";
            this.ErrorLabel.Size = new System.Drawing.Size(41, 13);
            this.ErrorLabel.TabIndex = 9;
            this.ErrorLabel.Text = "label4";
            this.ErrorLabel.Visible = false;
            // 
            // secretKeyTextBox
            // 
            this.secretKeyTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.secretKeyTextBox.Location = new System.Drawing.Point(105, 26);
            this.secretKeyTextBox.Name = "secretKeyTextBox";
            this.secretKeyTextBox.Size = new System.Drawing.Size(269, 20);
            this.secretKeyTextBox.TabIndex = 10;
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.CancelLoginButton);
            this.panel1.Controls.Add(this.OKButton);
            this.panel1.Controls.Add(this.ErrorLabel);
            this.panel1.Controls.Add(this.secretKeyTextBox);
            this.panel1.Controls.Add(this.DeveloperKeyTextBox);
            this.panel1.Controls.Add(this.GameSourceUrlTextBox);
            this.panel1.Controls.Add(this.RememberMeCheckBox);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.label2);
            this.panel1.Location = new System.Drawing.Point(10, 12);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(374, 122);
            this.panel1.TabIndex = 11;
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::MGDF.GamesManager.GameSource.DataLoader.Properties.Resources.loading;
            this.pictureBox1.Location = new System.Drawing.Point(174, 47);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(34, 35);
            this.pictureBox1.TabIndex = 12;
            this.pictureBox1.TabStop = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(162, 85);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(65, 13);
            this.label4.TabIndex = 13;
            this.label4.Text = "Logging in...";
            // 
            // LoginForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(395, 144);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.pictureBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(389, 172);
            this.Name = "LoginForm";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Login";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox DeveloperKeyTextBox;
        private System.Windows.Forms.TextBox GameSourceUrlTextBox;
        private System.Windows.Forms.CheckBox RememberMeCheckBox;
        private System.Windows.Forms.Button CancelLoginButton;
        private System.Windows.Forms.Button OKButton;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label ErrorLabel;
        private System.Windows.Forms.TextBox secretKeyTextBox;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label4;
    }
}