using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using MGDF.CrashReporter;

internal static partial class Program
{
  private static string _root;
  private static ReporterOptions _options;
  private static CrashResult _crash;
  private static FileStream _log;
  private static string _logPath;

  [STAThread]
  private static int Main(string[] args)
  {
    MGDF.Tools.ToolForm.EnableStyles();
    _root = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "reporter-tests-" + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(_root);
    _options = new ReporterOptions
    {
      GameName = "Test game", GameUid = "game-uid", GameVersion = "1.2.3", FrameworkVersion = "4.5.6",
      UserFolder = _root
    };

    _logPath = Path.Combine(_root, "corelog.txt");
    File.WriteAllText(_logPath, "A test log: \"error\".\n");
    _log = new FileStream(_logPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
    _options.LogHandle = _log.SafeFileHandle.DangerousGetHandle();
    _crash = new CrashResult { Failed = true, Detail = "Test \"failure\"\nwith detail." };
    Action[] tests = { ZipWithDump, ZipWithoutDump, UploadContract, RejectedService, MissingUploadUrl,
      InvalidResponse, HttpFailure, UploadFailure, CancelRequest, HiddenReportsAreBlocked, CleanExit, TaskManagerExit, ReturnedFailure, ExceptionExit, InterruptedExit, SignaledCrash, RetainedLog, FallbackError, ErrorTextHasFixedLayout, FullReportChain, SendStatesKeepButtonFocus, StalledUploadStops, MovingUploadHasNoTotalLimit };
    var only = args.FirstOrDefault(arg => arg.StartsWith("--only=", StringComparison.Ordinal));
    if (only != null) tests = tests.Where(test => test.Method.Name == only.Substring(7)).ToArray();
    Check(tests.Length != 0, "No tests selected.");
    var repeat = args.FirstOrDefault(arg => arg.StartsWith("--repeat=", StringComparison.Ordinal));
    if (repeat != null) tests = Enumerable.Range(0, int.Parse(repeat.Substring(9))).SelectMany(unused => tests).ToArray();
    if (args.Contains("--reverse")) Array.Reverse(tests);
    int failures = 0;
    try
    {
      foreach (var test in tests)
      {
        try { test(); Console.WriteLine("PASS " + test.Method.Name); }
        catch (Exception ex) { failures++; Console.WriteLine("FAIL " + test.Method.Name + ": " + ex); }
      }
      Console.WriteLine("Tests: " + tests.Length + ", passed: " + (tests.Length - failures) + ", failed: " + failures);
      return failures == 0 ? 0 : 1;
    }
    finally { _log.Dispose(); Directory.Delete(_root, true); }
  }

  private static void Check(bool value, string message)
  {
    if (!value) throw new Exception(message);
  }

  private static void ZipWithDump()
  {
    var dump = Path.Combine(_root, "minidump.dmp");
    File.WriteAllBytes(dump, new byte[] { 1, 2, 3 });
    _crash.DumpFile = dump;
    using (var output = new MemoryStream())
    {
      ReportUpload.WriteZip(output, _options, _crash);
      output.Position = 0;
      using (var zip = new ZipArchive(output, ZipArchiveMode.Read))
      {
        Check(zip.Entries.Count == 2, "Wrong zip entries.");
        using (var data = new MemoryStream())
        {
          zip.GetEntry("minidump.dmp").Open().CopyTo(data);
          Check(data.ToArray().SequenceEqual(new byte[] { 1, 2, 3 }), "Dump data changed.");
        }
        CheckLog(zip);
      }
    }
    _crash.DumpFile = null;
  }

  private static void ZipWithoutDump()
  {
    // A dump from a previous run must not enter a log-only report.
    using (var output = new MemoryStream())
    {
      ReportUpload.WriteZip(output, _options, _crash);
      output.Position = 0;
      using (var zip = new ZipArchive(output, ZipArchiveMode.Read))
      {
        Check(zip.Entries.Count == 1 && zip.GetEntry("minidump.dmp") == null, "Stale dump included.");
        CheckLog(zip);
      }
    }
  }

  private static void CheckLog(ZipArchive zip)
  {
    using (var reader = new StreamReader(zip.GetEntry("log.txt").Open()))
    {
      var log = reader.ReadToEnd();
      Check(log.Contains("MGDF Version: 4.5.6") && log.Contains("Game: game-uid 1.2.3") &&
        log.Contains(_crash.Detail) && log.Contains("A test log: \"error\"."), "Missing report data.");
    }
  }

  private static void UploadContract()
  {
    using (var server = new Server())
    {
      _options.ServiceUrl = server.Url + "crashes";
      var worker = Task.Run(() =>
      {
        using (var client = server.Accept())
        {
          var request = ReadRequest(client);
          Check(request.Line == "POST /crashes HTTP/1.1", "Wrong service method.");
          Check(request.Headers["Content-Type"] == "application/json", "Wrong JSON content type.");
          var json = new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(Encoding.UTF8.GetString(request.Body));
          Check(json.Count == 4 && (string)json["detail"] == _crash.Detail &&
            (string)json["gameUid"] == _options.GameUid && (string)json["gameVersion"] == _options.GameVersion &&
            (string)json["frameworkVersion"] == _options.FrameworkVersion, "Wrong request JSON.");
          Respond(client, "{\"success\":true,\"uploadUrl\":\"" + server.Url + "upload?signature=test\"}");
        }
        using (var client = server.Accept())
        {
          var request = ReadRequest(client);
          Check(request.Line == "PUT /upload?signature=test HTTP/1.1", "Wrong upload method or URL.");
          Check(request.Headers["Content-Type"] == "application/zip", "Wrong upload content type.");
          using (var zip = new ZipArchive(new MemoryStream(request.Body), ZipArchiveMode.Read)) CheckLog(zip);
          Respond(client, "");
        }
      });
      ReportUpload.SendAsync(_options, _crash, null, CancellationToken.None).GetAwaiter().GetResult();
      Complete(worker);
      Check(Directory.GetFiles(_root, "crash-report-*.zip").Length == 0, "Temporary report remains.");
    }
  }

  private static void RejectedService() { CheckBadResponse("{\"success\":false,\"uploadUrl\":\"http://127.0.0.1:1/\"}"); }
  private static void MissingUploadUrl() { CheckBadResponse("{\"success\":true}"); }
  private static void InvalidResponse() { CheckBadResponse("not JSON"); }
  private static void HttpFailure() { CheckBadResponse("{}", 500); }

  private static void CheckBadResponse(string response, int code = 200)
  {
    using (var server = new Server())
    {
      _options.ServiceUrl = server.Url;
      var worker = Task.Run(() =>
      {
        using (var client = server.Accept()) { ReadRequest(client); Respond(client, response, code); }
      });
      ExpectFailure(() => ReportUpload.SendAsync(_options, _crash, null, CancellationToken.None).GetAwaiter().GetResult());
      Complete(worker);
      Check(!server.Pending, "Rejected response started an upload.");
      Check(Directory.GetFiles(_root, "crash-report-*.zip").Length == 0, "Failed report remains.");
    }
  }

  private static void UploadFailure()
  {
    using (var server = new Server())
    {
      _options.ServiceUrl = server.Url;
      var worker = Task.Run(() =>
      {
        using (var client = server.Accept())
        {
          ReadRequest(client);
          Respond(client, "{\"success\":true,\"uploadUrl\":\"" + server.Url + "upload\"}");
        }
        using (var client = server.Accept()) { ReadRequest(client); Respond(client, "", 500); }
      });
      ExpectFailure(() => ReportUpload.SendAsync(_options, _crash, null, CancellationToken.None).GetAwaiter().GetResult());
      Complete(worker);
    }
  }

  private static void CancelRequest()
  {
    using (var server = new Server())
    using (var received = new ManualResetEventSlim())
    using (var release = new ManualResetEventSlim())
    using (var cancel = new CancellationTokenSource())
    {
      _options.ServiceUrl = server.Url;
      var worker = Task.Run(() =>
      {
        using (var client = server.Accept())
        {
          ReadRequest(client);
          received.Set();
          Check(release.Wait(TimeSpan.FromSeconds(10)), "Cancel test did not release server.");
        }
      });
      var upload = ReportUpload.SendAsync(_options, _crash, null, cancel.Token);
      try
      {
        Check(received.Wait(TimeSpan.FromSeconds(10)), "No service request.");
        cancel.Cancel();
        Check(Task.WhenAny(upload, Task.Delay(10000)).GetAwaiter().GetResult() == upload, "Cancel did not stop the request.");
        ExpectFailure(() => upload.GetAwaiter().GetResult());
      }
      finally { release.Set(); Complete(worker); }
    }
  }

  private static void HiddenReportsAreBlocked()
  {
    _options.HideErrors = true;
    try { ExpectFailure(() => ReportUpload.SendAsync(_options, _crash, null, CancellationToken.None).GetAwaiter().GetResult()); }
    finally { _options.HideErrors = false; }
  }

  [DllImport("kernel32.dll")]
  private static extern IntPtr GetCurrentProcess();
  [DllImport("kernel32.dll", SetLastError = true)]
  private static extern bool DuplicateHandle(IntPtr sourceProcess, IntPtr source, IntPtr targetProcess,
    out IntPtr target, uint access, bool inherit, uint options);
  [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
  private static extern SafeFileHandle CreateFileMapping(IntPtr file, IntPtr security, uint protect,
    uint high, uint low, string name);

  private static IntPtr Duplicate(IntPtr source)
  {
    Check(DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), out IntPtr target,
      0, false, 2), "Cannot copy test handle.");
    return target;
  }

  private static void CleanExit() { CheckExit(0, false); }
  private static void TaskManagerExit() { CheckExit(1, false); }
  private static void ReturnedFailure() { CheckExit(17, false); }
  private static void ExceptionExit() { CheckExit(0xC0000005, true); }
  private static void InterruptedExit() { CheckExit(0xC000013A, false); }
  private static void SignaledCrash() { CheckExit(0, true, true); }

  private static void CheckExit(uint code, bool failed, bool signal = false)
  {
    using (var process = Process.Start(new ProcessStartInfo(Environment.GetEnvironmentVariable("COMSPEC"),
      "/d /c exit /b " + unchecked((int)code)) { UseShellExecute = false, CreateNoWindow = true }))
    using (var crash = new EventWaitHandle(false, EventResetMode.ManualReset))
    using (var done = new EventWaitHandle(false, EventResetMode.ManualReset))
    using (var ready = new EventWaitHandle(false, EventResetMode.ManualReset))
    using (var mapping = CreateFileMapping(new IntPtr(-1), IntPtr.Zero, 4, 0, (uint)CrashLayout.Size, null))
    {
      Check(!mapping.IsInvalid, "Cannot create test mapping.");
      Check(process.WaitForExit(10000), "Child did not exit.");
      var options = new ReporterOptions
      {
        ProcessId = (uint)process.Id, ProcessHandle = Duplicate(process.Handle),
        CrashEvent = Duplicate(crash.SafeWaitHandle.DangerousGetHandle()),
        DumpEvent = Duplicate(done.SafeWaitHandle.DangerousGetHandle()),
        ReadyEvent = Duplicate(ready.SafeWaitHandle.DangerousGetHandle()),
        MappingHandle = Duplicate(mapping.DangerousGetHandle()), UserFolder = _root
      };
      if (signal) crash.Set();
      var result = CrashMonitor.Wait(options);
      Check(result.Failed == failed, "Wrong exit decision.");
      Check(result.DumpFile == null, "Process exit used a stale dump.");
      Check(result.Detail.Contains("Exit code: " + code + "."), "Missing exit code.");
      Check(ready.WaitOne(0), "Reporter did not signal ready.");
    }
  }

  [StructLayout(LayoutKind.Sequential)]
  private struct EditRect { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")]
  private static extern IntPtr SendMessage(IntPtr window, int message, IntPtr value, ref EditRect rect);
  [DllImport("user32.dll")]
  private static extern int GetWindowLong(IntPtr window, int index);

  private static void ErrorTextHasFixedLayout()
  {
    var longText = string.Join("\r\n", Enumerable.Repeat("A long error line with enough words to wrap in the error box.", 100));
    using (var shortForm = new ReportForm(_options, new CrashResult { ErrorText = "Renderer: No device." }))
    using (var longForm = new ReportForm(_options, new CrashResult { ErrorText = longText }))
    using (var emptyForm = new ReportForm(_options, new CrashResult()))
    {
      foreach (var form in new[] { shortForm, longForm, emptyForm })
      {
        form.ShowInTaskbar = false;
        form.StartPosition = FormStartPosition.Manual;
        form.Location = new System.Drawing.Point(-32000, -32000);
        form.Show();
        form.PerformLayout();
      }
      var box = longForm.Controls.OfType<TextBox>().Single();
      Check(box.Text == longText.Substring(0, (int)CrashLayout.ErrorLimit), "Error text was not limited.");
      Check(shortForm.ClientSize == longForm.ClientSize, "Error length changed the window size.");
      Check(shortForm.Controls.OfType<FlowLayoutPanel>().Single().Bounds == longForm.Controls.OfType<FlowLayoutPanel>().Single().Bounds, "Error length moved the buttons.");
      Check(emptyForm.ClientSize.Height < shortForm.ClientSize.Height && !emptyForm.Controls.OfType<TextBox>().Any(), "Empty error text did not remove its row.");
      Check(box.ReadOnly && box.Multiline && box.WordWrap && box.ScrollBars == ScrollBars.Vertical, "Error text cannot scroll and wrap.");
      var copy = longForm.Controls.OfType<Button>().Single();
      var buttons = longForm.Controls.OfType<FlowLayoutPanel>().Single();
      var body = longForm.Controls.OfType<LinkLabel>().Single();
      Check(copy.Text == "Copy" && copy.Left == box.Left && copy.Top == buttons.Top, "Copy is not at the left of the button row.");
      Check(!copy.Bounds.IntersectsWith(buttons.Bounds), "The button panel covers Copy.");
      Check(box.Left == body.Left && box.Width == body.Width, "Error text does not use the content width.");
      Check(!emptyForm.Controls.OfType<Button>().Any(), "Empty error text has a Copy button.");
      var rect = new EditRect();
      SendMessage(box.Handle, 0x00B2, IntPtr.Zero, ref rect);
      int lineHeight = box.GetPositionFromCharIndex(box.GetFirstCharIndexFromLine(1)).Y - box.GetPositionFromCharIndex(0).Y;
      Check(lineHeight > 0 && rect.Bottom - rect.Top == lineHeight * 4, "The error box does not show four whole lines.");
      Check((GetWindowLong(box.Handle, -16) & 0x00200000) != 0 && box.GetFirstCharIndexFromLine(4) >= 0, "Long error text has no native vertical scroll bar.");
    }
  }

  private static void RetainedLog()
  {
    File.Delete(_logPath);
    File.WriteAllText(_logPath, "A later run.");
    ZipWithoutDump();
  }

  private static void FallbackError()
  {
    int calls = 0;
    Check(MGDF.CrashReporter.Program.Run(new string[0], (text, title) => {
      Check(text == "The game stopped unexpectedly.", "Wrong fallback text."); calls++;
    }) == 1 && calls == 1, "Reporter failure gave no message.");
  }

  private static void ExpectFailure(Action action)
  {
    bool failed = false;
    try { action(); }
    catch (Exception) { failed = true; }
    Check(failed, "Expected failure was not reported.");
  }

  private static void Complete(Task task)
  {
    Check(Task.WhenAny(task, Task.Delay(10000)).GetAwaiter().GetResult() == task, "Server timed out.");
    task.GetAwaiter().GetResult();
  }

  private sealed class Server : IDisposable
  {
    private readonly TcpListener _listener = new TcpListener(IPAddress.Loopback, 0);
    public Server() { _listener.Start(); }
    public string Url => "http://127.0.0.1:" + ((IPEndPoint)_listener.LocalEndpoint).Port + "/";
    public bool Pending => _listener.Pending();
    public TcpClient Accept()
    {
      var pending = _listener.AcceptTcpClientAsync();
      Check(Task.WhenAny(pending, Task.Delay(10000)).GetAwaiter().GetResult() == pending, "No HTTP request.");
      var client = pending.GetAwaiter().GetResult();
      client.ReceiveTimeout = client.SendTimeout = 10000;
      return client;
    }
    public void Dispose() { _listener.Stop(); }
  }

  private sealed class Request
  {
    public string Line;
    public Dictionary<string, string> Headers = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    public byte[] Body;
  }

  private static Request ReadRequest(TcpClient client)
  {
    var stream = client.GetStream();
    var header = new StringBuilder();
    while (!header.ToString().EndsWith("\r\n\r\n", StringComparison.Ordinal))
    {
      int value = stream.ReadByte();
      Check(value >= 0 && header.Length < 65536, "Incomplete HTTP header.");
      header.Append((char)value);
    }
    var lines = header.ToString().Split(new[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries);
    var request = new Request { Line = lines[0] };
    foreach (var line in lines.Skip(1))
    {
      int colon = line.IndexOf(':');
      request.Headers.Add(line.Substring(0, colon), line.Substring(colon + 1).Trim());
    }
    request.Body = new byte[int.Parse(request.Headers["Content-Length"])];
    int offset = 0;
    while (offset < request.Body.Length)
    {
      int count = stream.Read(request.Body, offset, request.Body.Length - offset);
      Check(count != 0, "Incomplete HTTP body.");
      offset += count;
    }
    return request;
  }

  private static void Respond(TcpClient client, string body, int code = 200)
  {
    byte[] data = Encoding.UTF8.GetBytes(body);
    byte[] header = Encoding.ASCII.GetBytes("HTTP/1.1 " + code + " Result\r\nContent-Type: application/json\r\nContent-Length: " + data.Length + "\r\nConnection: close\r\n\r\n");
    client.GetStream().Write(header, 0, header.Length);
    client.GetStream().Write(data, 0, data.Length);
  }
}
