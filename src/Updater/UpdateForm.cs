using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using MGDF.Tools;

namespace MGDF.Updater
{
  public sealed class UpdateForm : ToolForm
  {
    private readonly UpdateSession _session;
    private readonly UpdateInfo _update;
    private CancellationTokenSource _download;
    private bool _handedOff;
    private bool _busy;

    public UpdateForm(UpdateSession session, UpdateInfo update, string iconPath)
      : base(session.Game.gameName, iconPath, "An update for " + session.Game.gameName + " is available",
        "", "Update now", "Later")
    {
      _session = session;
      _update = update;
      if (session.RepairRequired) { Body.Text = "The update did not finish. Install the update to repair the game."; Secondary.Text = "Close"; }
      Primary.Click += Download;
      Secondary.Click += (sender, e) => { if (_busy) _download.Cancel(); else Close(); };
    }

    private async void Download(object sender, EventArgs e)
    {
      _busy = true;
      Primary.Visible = false;
      Secondary.Text = "Cancel";
      Progress.Visible = true;
      Body.Text = "Downloading update...";
      using (var cancel = new CancellationTokenSource())
      {
        _download = cancel;
        try
        {
          var progress = new Progress<int>(value => { if (!IsDisposed) Progress.Value = value; });
          await _session.StageAsync(_update, progress, cancel.Token);
          _handedOff = true;
        }
        catch (Exception) { if (!cancel.IsCancellationRequested) _session.ShowDownloadFailure(); }
        finally { _busy = false; _download = null; }
      }
      Close();
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
      if (_busy) { _download.Cancel(); e.Cancel = true; }
      else if (!_handedOff) _session.StartGame();
      base.OnFormClosing(e);
    }
  }
}
