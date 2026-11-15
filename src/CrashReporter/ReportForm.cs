using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MGDF.CrashReporter
{
  public sealed class ReportForm : Form
  {
    private readonly ReporterOptions _options;
    private readonly CrashResult _crash;
    private readonly Button _send;
    private readonly Button _cancel;
    private readonly Label _status;
    private readonly ProgressBar _progress;
    private readonly PictureBox _picture;
    private CancellationTokenSource _upload;

    public ReportForm(ReporterOptions options, CrashResult crash)
    {
      _options = options;
      _crash = crash;
      Text = options.GameName;
      Font = new Font("Segoe UI", 9F);
      AutoScaleDimensions = new SizeF(96, 96);
      AutoScaleMode = AutoScaleMode.Dpi;
      ClientSize = new Size(440, 208);
      FormBorderStyle = FormBorderStyle.FixedDialog;
      MaximizeBox = false;
      MinimizeBox = false;
      StartPosition = FormStartPosition.CenterScreen;
      _picture = new PictureBox
      {
        Location = new Point(20, 22), Size = new Size(48, 48), SizeMode = PictureBoxSizeMode.Zoom
      };
      try
      {
        if (File.Exists(options.IconPath))
        {
          using (var image = Image.FromFile(options.IconPath)) _picture.Image = new Bitmap(image);
        }
      }
      catch (Exception ex) when (ex is IOException || ex is ArgumentException || ex is UnauthorizedAccessException)
      { }
      var title = new Label
      {
        Text = options.GameName, AutoEllipsis = true, Location = new Point(84, 21),
        Size = new Size(336, 26), Font = new Font(Font, FontStyle.Bold)
      };
      _status = new Label
      {
        Text = "The game stopped unexpectedly.", Location = new Point(84, 51), Size = new Size(336, 38)
      };
      var privacy = new LinkLabel
      {
        Text = "Privacy policy", AutoSize = true, Location = new Point(20, 101),
        Visible = !string.IsNullOrEmpty(options.PrivacyPolicyUrl)
      };
      privacy.LinkClicked += OpenPrivacyPolicy;
      _progress = new ProgressBar
      {
        Location = new Point(20, 133), Size = new Size(400, 8), Visible = false
      };
      _send = new Button { Text = "Send report", Location = new Point(194, 164), Size = new Size(108, 28) };
      _cancel = new Button { Text = "Don't send", Location = new Point(312, 164), Size = new Size(108, 28) };
      _send.Click += SendReport;
      _cancel.Click += (sender, e) => Close();
      AcceptButton = _send;
      CancelButton = _cancel;
      Controls.AddRange(new Control[] { _picture, title, _status, privacy, _progress, _send, _cancel });
    }

    private void OpenPrivacyPolicy(object sender, LinkLabelLinkClickedEventArgs e)
    {
      if (!Uri.TryCreate(_options.PrivacyPolicyUrl, UriKind.Absolute, out Uri uri) ||
          (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps)) return;
      try { Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true }); }
      catch (System.ComponentModel.Win32Exception) { _status.Text = "Cannot open the privacy policy."; }
    }

    private async void SendReport(object sender, EventArgs e)
    {
      _send.Visible = false;
      _cancel.Text = "Cancel";
      _progress.Visible = true;
      _status.Text = "Sending report...";
      var upload = new CancellationTokenSource(TimeSpan.FromMinutes(2));
      _upload = upload;
      try
      {
        var progress = new Progress<int>(value => { if (!IsDisposed) _progress.Value = value; });
        await Task.Run(() => ReportUpload.SendAsync(_options, _crash, progress, upload.Token));
        if (!IsDisposed) Close();
      }
      catch (Exception)
      {
        if (!IsDisposed)
        {
          _status.Text = "Cannot send the report. Please try again later.";
          _progress.Visible = false;
          _cancel.Text = "Close";
        }
      }
      finally
      {
        _upload = null;
        upload.Dispose();
      }
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
      _upload?.Cancel();
      base.OnFormClosing(e);
    }

    protected override void Dispose(bool disposing)
    {
      if (disposing) _picture.Image?.Dispose();
      base.Dispose(disposing);
    }
  }
}
