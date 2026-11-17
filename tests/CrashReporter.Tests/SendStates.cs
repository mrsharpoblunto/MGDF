using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using MGDF.CrashReporter;

internal static partial class Program
{
  private static void SendStatesKeepButtonFocus()
  {
    // Hiding Send report must not select the error text.
    foreach (var error in new[] { null, "Renderer: No DirectX 12 device." })
    using (var server = new Server())
    using (var release = new ManualResetEventSlim())
    using (var form = new ReportForm(new ReporterOptions {
      GameName = "Focus test", UserFolder = _root, ServiceUrl = server.Url
    }, new CrashResult { ErrorText = error }))
    using (var timer = new System.Windows.Forms.Timer { Interval = 10000 })
    {
      Exception failure = null;
      bool sending = false, failed = false;
      form.ShowInTaskbar = false;
      form.StartPosition = FormStartPosition.Manual;
      form.Location = new System.Drawing.Point(-32000, -32000);
      var buttons = form.Controls.OfType<FlowLayoutPanel>().Single().Controls.OfType<Button>().ToArray();
      var send = buttons.Single(button => button.Text == "Send report");
      var close = buttons.Single(button => button.Text == "Don't send");
      var body = form.Controls.OfType<LinkLabel>().Single();
      var worker = Task.Run(() => {
        using (var client = server.Accept())
        {
          ReadRequest(client);
          Check(release.Wait(TimeSpan.FromSeconds(10)), "Focus test did not release the server.");
          Respond(client, "{\"success\":false}");
        }
      });
      Action<Action> check = action => {
        try { action(); }
        catch (Exception ex) { failure = ex; form.Close(); }
      };
      form.Shown += (sender, e) => {
        timer.Start();
        form.BeginInvoke(new Action(() => check(() => {
          send.PerformClick();
          Check(!send.Visible && close.Text == "Cancel" && form.ActiveControl == close && form.AcceptButton == close,
            "Sending state did not focus Cancel.");
          sending = true;
          release.Set();
        })));
      };
      body.TextChanged += (sender, e) => {
        if (!body.Text.StartsWith("Cannot send", StringComparison.Ordinal)) return;
        form.BeginInvoke(new Action(() => check(() => {
          Check(close.Text == "Close" && form.ActiveControl == close && form.AcceptButton == close,
            "Failed state did not focus Close.");
          Check(!form.Controls.OfType<TextBox>().Any(box => box.Focused), "Error text has focus.");
          failed = true;
          close.PerformClick();
        })));
      };
      timer.Tick += (sender, e) => { failure = new Exception("Focus test timed out."); form.Close(); };
      try { Application.Run(form); }
      finally { timer.Stop(); release.Set(); Complete(worker); }
      if (failure != null) throw failure;
      Check(sending && failed, "Focus test missed a form state.");
    }
  }

  private static void StalledUploadStops() { CheckUploadProgress(false); }
  private static void MovingUploadHasNoTotalLimit() { CheckUploadProgress(true); }

  private static void CheckUploadProgress(bool moving)
  {
    // A stopped socket must time out; continued writes must reset that limit.
    using (var server = new Server())
    using (var received = new ManualResetEventSlim())
    using (var release = new ManualResetEventSlim())
    using (var cancel = new CancellationTokenSource())
    {
      var dump = Path.Combine(_root, "upload-data.dmp");
      var data = new byte[16 * 1024 * 1024];
      new Random(42).NextBytes(data);
      File.WriteAllBytes(dump, data);
      var options = new ReporterOptions { UserFolder = _root, ServiceUrl = server.Url };
      var crash = new CrashResult { DumpFile = dump };
      var interval = TimeSpan.FromSeconds(2);
      var watch = Stopwatch.StartNew();
      var worker = Task.Run(async () => {
        using (var client = server.Accept())
        {
          ReadRequest(client);
          Respond(client, "{\"success\":true,\"uploadUrl\":\"" + server.Url + "upload\"}");
        }
        using (var client = server.Accept())
        {
          var stream = client.GetStream();
          var header = new StringBuilder();
          while (!header.ToString().EndsWith("\r\n\r\n", StringComparison.Ordinal))
          {
            int value = stream.ReadByte();
            Check(value >= 0 && header.Length < 65536, "Missing upload header.");
            header.Append((char)value);
          }
          int length = int.Parse(header.ToString().Split(new[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries)
            .Single(line => line.StartsWith("Content-Length:", StringComparison.OrdinalIgnoreCase)).Substring(15));
          received.Set();
          if (!moving)
          {
            Check(release.Wait(TimeSpan.FromSeconds(15)), "Stall test did not release the server.");
            return;
          }
          var buffer = new byte[65536];
          int total = 0;
          while (total < length)
          {
            int count = stream.Read(buffer, 0, Math.Min(buffer.Length, length - total));
            Check(count > 0, "Moving upload stopped early.");
            total += count;
            // This delay sets the transfer rate. It does not wait for a worker to start.
            await Task.Delay(20, cancel.Token).ConfigureAwait(false);
          }
          Respond(client, "");
        }
      });
      var upload = ReportUpload.SendAsync(options, crash, null, cancel.Token, interval);
      try
      {
        Check(received.Wait(TimeSpan.FromSeconds(10)), "Upload did not start.");
        Check(Task.WhenAny(upload, Task.Delay(15000)).GetAwaiter().GetResult() == upload, "Upload did not stop.");
        if (moving)
        {
          upload.GetAwaiter().GetResult();
          Check(watch.Elapsed > interval + interval, "Upload did not run beyond its inactivity limit.");
        }
        else ExpectFailure(() => upload.GetAwaiter().GetResult());
        Check(Directory.GetFiles(_root, "crash-report-*.zip").Length == 0, "Upload left a report zip.");
      }
      finally
      {
        release.Set();
        cancel.Cancel();
        Complete(worker);
        File.Delete(dump);
      }
    }
  }
}
