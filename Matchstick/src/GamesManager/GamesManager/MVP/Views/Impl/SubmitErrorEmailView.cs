using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views.Impl
{
  partial class SubmitErrorEmailView : GamesManagerViewBase, ISubmitErrorEmailView
  {
    public SubmitErrorEmailView()
    {
      InitializeComponent();
    }

    public event EventHandler CopyLogOutput;
    public event EventHandler EmailLogOutput;

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

    public string SupportEmail
    {
      set { label9.Text = "Click 'create' to create an email message to " + value; }
    }

    private void button1_Click(object sender, EventArgs e)
    {
      CopyLogOutput?.Invoke(this, new EventArgs());
    }

    private void button2_Click(object sender, EventArgs e)
    {
      EmailLogOutput?.Invoke(this, new EventArgs());
    }

    private void button3_Click(object sender, EventArgs e)
    {
      CloseView();
    }
  }
}