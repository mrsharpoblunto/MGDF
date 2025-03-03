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
  partial class SubmitErrorS3View : GamesManagerViewBase, ISubmitErrorS3View
  {
    public SubmitErrorS3View()
    {
      InitializeComponent();
      AcceptButton = button1;
      CancelButton = button3;
    }

    public event EventHandler SendLogOutput;

    public bool Sending
    {
      set
      {
        pictureBox2.Visible = value;
        button1.Enabled = !value;
        button3.Enabled = !value;
        button1.Text = value ? "Sending..." : "Send Error Report";
      }
    }

    protected override void ShowView()
    {
      ShowDialog();
    }

    protected override void ShowView(IWin32Window owner)
    {
      ShowDialog(owner);
    }

    private void button3_Click(object sender, EventArgs e)
    {
      CloseView();
    }

    private void button1_Click_1(object sender, EventArgs e)
    {
      SendLogOutput?.Invoke(this, new EventArgs());
    }
  }
}