using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace MGDF.GamesManager.Controls
{
  class Message : IMessage
  {
    public event EventHandler Shown;

    public event CancelEventHandler Closing
    {
      add { throw new NotSupportedException(); }
      remove { }
    }

    public event EventHandler Closed;

    public event EventHandler<COPYDATASTRUCT> OnWindowCopyData
    {
      add { throw new NotSupportedException(); }
      remove { }
    }

    public bool Enabled
    {
      get { return true; }
      set { }
    }

    public long WindowHandle => 0;

    public void ShowView(IView parentView)
    {
    }

    public void CloseView()
    {
    }

    public void Invoke(InvokeDelegate d)
    {
      d?.Invoke();
    }

    public void Show(string text)
    {
      Shown?.Invoke(this, new EventArgs());
      MessageBox.Show(text);
      Closed?.Invoke(this, new EventArgs());
    }

    public void Show(string text, string caption)
    {
      Shown?.Invoke(this, new EventArgs());
      MessageBox.Show(text, caption);
      Closed?.Invoke(this, new EventArgs());
    }

    public DialogResult Show(string text, string caption, MessageBoxButtons buttons)
    {
      Shown?.Invoke(this, new EventArgs());
      var result = MessageBox.Show(text, caption, buttons);
      Closed?.Invoke(this, new EventArgs());
      return result;
    }
  }
}
