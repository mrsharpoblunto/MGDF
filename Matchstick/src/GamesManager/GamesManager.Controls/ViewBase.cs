using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls.Properties;

namespace MGDF.GamesManager.Controls
{
    public partial class ViewBase : Form, IView
    {
        public new event EventHandler Shown;
        public new event EventHandler Closed;
        public new event CancelEventHandler Closing;

        private readonly float _scaleFactorX;
        private readonly float _scaleFactorY;

        protected int DpiScaleX(int x)
        {
            return (int)(x * _scaleFactorX);
        }

        protected int DpiScaleY(int y)
        {
            return (int)(y * _scaleFactorY);
        }

        public ViewBase()
        {
            InitializeComponent();
            Icon = Resources.MGDF;
            base.Shown += ViewBase_Shown;
            base.Closed += ViewBase_Closed;
            base.Closing += ViewBase_Closing;

            using (Graphics g = CreateGraphics())
            {
                _scaleFactorX = g.DpiX / 96;
                _scaleFactorY = g.DpiY / 96;
            }
        }

        void ViewBase_Closing(object sender, CancelEventArgs e)
        {
            if (Closing!=null)
            {
                Closing(this,e);
            }
        }

        void ViewBase_Shown(object sender, EventArgs e)
        {
            if (Shown != null)
            {
                Shown(this, e);
            }
        }

        public void Invoke(InvokeDelegate d)
        {
            base.Invoke(d);
        }

        public void ShowView(IView parentView)
        {
            if (parentView is IWin32Window)
            {
                ShowView(parentView as IWin32Window);
            }
            else if (parentView == null)
            {
                ShowView();
            }
        }

        protected virtual void ShowView(IWin32Window owner) { Show(owner); }
        protected virtual void ShowView() { Show(); }

        public virtual void CloseView()
        {
            Close();
        }

        private void ViewBase_Closed(object sender, EventArgs e)
        {
            if (Closed != null)
            {
                Closed(this, new EventArgs());
            }
        }
    }
}
