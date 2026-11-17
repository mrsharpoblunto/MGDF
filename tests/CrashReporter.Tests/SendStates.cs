using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using MGDF.CrashReporter;

internal static partial class Program
{
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
