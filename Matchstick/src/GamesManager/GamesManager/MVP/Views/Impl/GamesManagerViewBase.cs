using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    class GamesManagerViewBase: ViewBase
    {
        public GamesManagerViewBase()
        {
            SetStyle(
                ControlStyles.UserPaint |
                ControlStyles.AllPaintingInWmPaint |
                ControlStyles.OptimizedDoubleBuffer, true);
        }

        protected new Image BackgroundImage { set; private get; }
    
        protected override void OnPaintBackground(PaintEventArgs e)
        {
            Graphics g = e.Graphics;


            base.OnPaintBackground(e);

            Rectangle rect = new Rectangle(ClientRectangle.X, ClientRectangle.Y, ClientRectangle.Width, DpiScaleY(64));
            using (SolidBrush headerBrush = new SolidBrush(Color.Black))
            {
                g.FillRectangle(headerBrush, rect);
            }

            if (BackgroundImage != null)
            {
                g.DrawImage(BackgroundImage, ClientRectangle.X, DpiScaleY(64) + 1, ClientRectangle.Width, ClientRectangle.Height - (DpiScaleY(64) + 2));
            }
        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // GamesManagerViewBase
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.ClientSize = new System.Drawing.Size(284, 262);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "GamesManagerViewBase";
            this.ShowIcon = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.ResumeLayout(false);

        }
    }
}