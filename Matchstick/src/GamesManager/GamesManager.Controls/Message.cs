using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MGDF.GamesManager.Controls
{
    class Message: IMessage
    {
        public event EventHandler Shown;
        public event CancelEventHandler Closing;
        public event EventHandler Closed;

        public bool Enabled
        {
            get { return true; }
            set {  }
        }

        public void ShowView(IView parentView)
        {
        }

        public void CloseView()
        {
        }

        public void Invoke(InvokeDelegate d)
        {
            d();
        }

        public void Show(string text)
        {
            if (Shown!=null)
            {
                Shown(this,new EventArgs());
            }
            MessageBox.Show(text);
            if (Closed != null)
            {
                Closed(this, new EventArgs());
            }
        }

        public void Show(string text, string caption)
        {
            if (Shown != null)
            {
                Shown(this, new EventArgs());
            }
            MessageBox.Show(text, caption);
            if (Closed != null)
            {
                Closed(this, new EventArgs());
            }
        }

        public DialogResult Show(string text, string caption, MessageBoxButtons buttons)
        {
            if (Shown != null)
            {
                Shown(this, new EventArgs());
            }
            var result = MessageBox.Show(text, caption, buttons); 
            if (Closed != null)
            {
                Closed(this, new EventArgs());
            }
            return result;
        }
    }
}
