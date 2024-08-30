using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views.Impl
{
  partial class SubmitErrorEmailView : GamesManagerViewBase, ISubmitErrorView
  {
    private string _supportUrl;
    private string _supportType;

    public SubmitErrorEmailView()
    {
      InitializeComponent();
    }

    public event EventHandler CopyLogOutput;
    public event EventHandler SendLogOutput;

    protected override void ShowView()
    {
      ShowDialog();
    }

    protected override void ShowView(IWin32Window owner)
    {
      ShowDialog(owner);
    }

    public string Message
    {
      set { textBox1.Text = value; }
    }

    public string SupportType
    {
      set
      {
        _supportType = value;
        if (value == "GitHub")
        {
          label10.Text = "Paste the contents of the clipboard into the message and submit it";
        }
        else
        {
          label10.Text = "Paste the contents of the clipboard into the message and send it";
        }
      }
      get { return _supportType; }
    }

    public string SupportUrl
    {
      set
      {
        _supportUrl = value;
        if (_supportType == "GitHub")
        {
          label9.Text = "Click 'create' to create a GitHub issue";
        }
        else
        {
          label9.Text = "Click 'create' to create an email message to " + value;
        }
      }
      get { return _supportUrl; }
    }

    private void button1_Click(object sender, EventArgs e)
    {
      CopyLogOutput?.Invoke(this, new EventArgs());
    }

    private void button2_Click(object sender, EventArgs e)
    {
      SendLogOutput?.Invoke(this, new EventArgs());
    }

    private void button3_Click(object sender, EventArgs e)
    {
      CloseView();
    }
  }
}