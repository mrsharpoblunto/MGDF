using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using MGDF.Tools;

namespace MGDF.CrashReporter
{
  public sealed class ReportForm : ToolForm
  {
    private readonly ReporterOptions _options;
    private readonly CrashResult _crash;
    private CancellationTokenSource _upload;

    public ReportForm(ReporterOptions options, CrashResult crash)
      : base(options.GameName, options.IconPath, options.GameName + " stopped unexpectedly",
        "Sorry about that. To help us fix the problem, you can send a crash report. The report contains " +
        "technical details of the crash, the game's log file and basic information about your system." +
        (string.IsNullOrEmpty(options.PrivacyPolicyUrl) ? "" : " For how we use this information, see our privacy policy."),
        "Send report", "Don't send")
    {
      _options = options;
      _crash = crash;
      if (!string.IsNullOrEmpty(options.PrivacyPolicyUrl))
        Body.LinkArea = new LinkArea(Body.Text.LastIndexOf("privacy policy", StringComparison.Ordinal), "privacy policy".Length);
      Body.LinkClicked += OpenPrivacyPolicy;
      Primary.Click += SendReport;
      Secondary.Click += (sender, e) => Close();
    }

    private void OpenPrivacyPolicy(object sender, LinkLabelLinkClickedEventArgs e)
    {
      if (!Uri.TryCreate(_options.PrivacyPolicyUrl, UriKind.Absolute, out Uri uri) ||
          (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps)) return;
      try { Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true }); }
      catch (System.ComponentModel.Win32Exception) { Body.Text = "Cannot open the privacy policy."; Body.LinkArea = new LinkArea(0, 0); }
    }

    private async void SendReport(object sender, EventArgs e)
    {
      Primary.Visible = false;
      Secondary.Text = "Cancel";
      Progress.Visible = true;
      Body.LinkArea = new LinkArea(0, 0);
      Body.Text = "Sending report...";
      var upload = new CancellationTokenSource(TimeSpan.FromMinutes(2));
      _upload = upload;
      try
      {
        var progress = new Progress<int>(value => { if (!IsDisposed) Progress.Value = value; });
        await Task.Run(() => ReportUpload.SendAsync(_options, _crash, progress, upload.Token));
        if (!IsDisposed) Close();
      }
      catch (Exception)
      {
        if (!IsDisposed)
        {
          Body.Text = "Cannot send the report. Please try again later.";
          Progress.Visible = false;
          Secondary.Text = "Close";
        }
      }
      finally { _upload = null; upload.Dispose(); }
    }

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
      _upload?.Cancel();
      base.OnFormClosing(e);
    }
  }
}
