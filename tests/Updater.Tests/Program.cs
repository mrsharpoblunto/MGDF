using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using MGDF.Updater;

internal static class Program
{
  private static string _root;
  private static string _bin;
  private static readonly System.Collections.Generic.List<UpdateSession> Sessions = new System.Collections.Generic.List<UpdateSession>();
  private static readonly JavaScriptSerializer Json = new JavaScriptSerializer();

  [STAThread]
  private static int Main(string[] args)
  {
    _bin = AppDomain.CurrentDomain.BaseDirectory;
    _root = Path.Combine(_bin, "updater-tests-" + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(_root);
    Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
    MGDF.Tools.ToolForm.EnableStyles();
    Action[] tests = { CoreFlags, NoService, ServiceFailure, SameVersion, PromptLayout, DownloadInactivity, DownloadKeepsReceiving, CancelDownload, HashMismatch,
      LockedInstaller, OneUpdater, PendingInstaller, PendingFailure, CompletedMarker, InUseFiles, CorruptManifestOffersRepair };
    if (args.Contains("--reverse")) Array.Reverse(tests);
    int failures = 0;
    try
    {
      foreach (var test in tests)
      {
        try { test(); Console.WriteLine("PASS " + test.Method.Name); }
        catch (Exception ex) { failures++; Console.WriteLine("FAIL " + test.Method.Name + ": " + ex); }
        finally { foreach (var session in Sessions) session.Dispose(); Sessions.Clear(); }
      }
      Console.WriteLine("Tests: " + tests.Length + ", passed: " + (tests.Length - failures) + ", failed: " + failures);
      return failures == 0 ? 0 : 1;
    }
    finally { Directory.Delete(_root, true); }
  }

  private static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
  private static void Complete(Task task)
  {
    Check(Task.WhenAny(task, Task.Delay(15000)).GetAwaiter().GetResult() == task, "Test operation timed out.");
    task.GetAwaiter().GetResult();
  }
  private static void Fails(Action action)
  {
    try { action(); }
    catch (Exception) { return; }
    throw new Exception("Expected a failure.");
  }
  private static string NewGame(string service = null)
  {
    var folder = Path.Combine(_root, "game with spaces " + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(Path.Combine(folder, "game"));
    File.WriteAllText(Path.Combine(folder, "game", "game.json"), Json.Serialize(new GameInfo
      { gameUid = "test", gameName = "Test game", version = "1.0", updateService = service }));
    File.Copy(Path.Combine(_bin, "updater.exe"), Path.Combine(folder, "updater.exe"));
    return folder;
  }
  private static UpdateSession Session(string folder, FakeProcesses processes)
  {
    var session = new UpdateSession(folder, Path.Combine(folder, "updater.exe"), new[] { "a b" }, processes, _root);
    Sessions.Add(session); return session;
  }
  private static UpdateInfo Update(string url, byte[] bytes)
  {
    using (var hash = SHA256.Create()) return new UpdateInfo
    {
      version = "2.0", installerUrl = url, sha256 = BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-", ""),
      installerArguments = new[] { "/S", "/D={installDir}" }
    };
  }
  private static void CoreFlags()
  {
    // Core reads flags from the raw command line.
    Check(UpdateProcesses.Quote("-hideerrors") == "-hideerrors", "Core flag was quoted.");
  }

  private static void NoService()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    Check(session.CheckAsync().Result == null, "No service returned an update.");
    Check(process.Games == 1, "No service did not start the game.");
  }
  private static void ServiceFailure()
  {
    using (var server = new Server())
    {
      var worker = server.Respond(new byte[0], 500);
      var process = new FakeProcesses(); var session = Session(NewGame(server.Url), process);
      Check(session.CheckAsync().Result == null, "Failed service returned an update.");
      Check(process.Games == 1, "Service failure did not start the game."); Complete(worker);
    }
  }
  private static void SameVersion()
  {
    using (var server = new Server())
    {
      var update = Update(server.Url, new byte[] { 1 }); update.version = "1.0.0.0";
      var worker = server.Respond(Encoding.UTF8.GetBytes(Json.Serialize(update)));
      var process = new FakeProcesses(); var session = Session(NewGame(server.Url), process);
      Check(session.CheckAsync().Result == null && process.Games == 1, "Equal versions did not start the game."); Complete(worker);
    }
  }
  private static void PromptLayout()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    var update = Update("https://example.invalid/", new byte[] { 1 });
    using (var form = new UpdateForm(session, update, ""))
    {
      form.PerformLayout();
      Check(form.Controls.OfType<LinkLabel>().Single().Text == "", "Update prompt has body text.");
      Check(!form.Controls.OfType<Label>().Any(label => label.Text.Contains(update.version)), "Update prompt shows the version.");
      int promptHeight = form.ClientSize.Height;
      new PendingInstall { Update = update }.Write(session.StateFolder);
      using (var repair = new UpdateForm(session, update, ""))
      {
        repair.PerformLayout();
        var body = repair.Controls.OfType<LinkLabel>().Single().Text;
        Check(body.Contains("repair the game") && !body.Contains(update.version), "Repair text lost its meaning or shows the version.");
        Check(repair.ClientSize.Height > promptHeight, "Update prompt kept the empty body space.");
      }
    }
  }
  private static void DownloadInactivity()
  {
    using (var server = new Server())
    using (var received = new ManualResetEventSlim())
    using (var release = new ManualResetEventSlim())
    {
      var worker = server.Stall(received, release);
      var output = Path.Combine(_root, "idle.exe");
      var download = UpdateNetwork.DownloadAsync(Update(server.Url, new byte[] { 1 }), output, null,
        CancellationToken.None, TimeSpan.FromMilliseconds(200));
      try
      {
        Check(received.Wait(10000), "No download request.");
        Check(Task.WhenAny(download, Task.Delay(10000)).Result == download, "An idle download did not stop.");
        Fails(() => download.GetAwaiter().GetResult());
      }
      finally { release.Set(); Complete(worker); }
    }
  }
  private static void DownloadKeepsReceiving()
  {
    using (var server = new Server())
    {
      var bytes = new byte[6 * 16384];
      var worker = server.Respond(bytes, chunks: 6);
      var output = Path.Combine(_root, "active.exe");
      UpdateNetwork.DownloadAsync(Update(server.Url, bytes), output, null,
        CancellationToken.None, TimeSpan.FromSeconds(2)).GetAwaiter().GetResult();
      Complete(worker);
      Check(File.ReadAllBytes(output).SequenceEqual(bytes), "A download with data stopped at the total time limit.");
    }
  }
  private sealed class CancelProgress : IProgress<int>
  {
    public CancellationTokenSource Cancel;
    public void Report(int value) { Cancel.Cancel(); }
  }
  private static void CancelDownload()
  {
    using (var server = new Server())
    using (var cancel = new CancellationTokenSource())
    {
      var bytes = new byte[131072]; var worker = server.Respond(bytes, 200, true);
      var process = new FakeProcesses(); var session = Session(NewGame(), process);
      Fails(() => session.StageAsync(Update(server.Url, bytes), new CancelProgress { Cancel = cancel }, cancel.Token).GetAwaiter().GetResult());
      session.StartGame(); Check(process.Helpers == 0 && process.Games == 1, "Cancelled download started an install.");
      Complete(worker); session.CleanOldFiles(); Check(Directory.GetDirectories(session.TempRoot).Length == 0, "Cancelled download remains.");
    }
  }
  private static void HashMismatch()
  {
    using (var server = new Server())
    {
      var worker = server.Respond(new byte[] { 1, 2 });
      var process = new FakeProcesses(); var session = Session(NewGame(), process);
      Fails(() => session.StageAsync(Update(server.Url, new byte[] { 3 }), null, CancellationToken.None).GetAwaiter().GetResult());
      Check(process.Helpers == 0, "Hash mismatch started a helper."); Complete(worker);
    }
  }
  private static void LockedInstaller()
  {
    var file = Path.Combine(_root, "locked.exe"); byte[] bytes = { 1, 2 }; File.WriteAllBytes(file, bytes);
    using (InstallerVerification.Open(file, Update("https://example.invalid", bytes).sha256))
    {
      Fails(() => { using (File.Open(file, FileMode.Open, FileAccess.Write, FileShare.ReadWrite)) { } });
      Fails(() => File.Move(file, file + ".moved"));
    }
  }
  private static void OneUpdater()
  {
    var folder = NewGame(); var first = Session(folder, new FakeProcesses());
    var process = new FakeProcesses(); var second = Session(folder + Path.DirectorySeparatorChar + ".", process);
    Check(first.Lock != null && second.Lock == null, "Two updaters acquired one install folder.");
    Check(second.CheckAsync().Result == null && process.Games == 0 && process.Failures == 1, "Second updater started a game.");
  }
  private static void PendingInstaller()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    using (var installer = Process.GetCurrentProcess())
      new PendingInstall { Update = Update("https://example.invalid", new byte[] { 1 }), ProcessId = installer.Id,
        ProcessStart = installer.StartTime.ToUniversalTime().Ticks }.Write(session.StateFolder);
    Check(session.CheckAsync().Result == null && process.Games == 0, "Live installer allowed a game start.");
    session.StartGame(); Check(process.Games == 0, "Pending installer was bypassed.");
  }
  private static void PendingFailure()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    new PendingInstall { Update = Update("https://example.invalid", new byte[] { 1 }) }.Write(session.StateFolder);
    Check(session.CheckAsync().Result?.version == "2.0", "Incomplete install did not offer repair after a service failure.");
    session.StartGame(); Check(process.Games == 0, "Incomplete install started the game.");
  }
  private static void CompletedMarker()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    var update = Update("https://example.invalid", new byte[] { 1 }); update.version = "1.0";
    new PendingInstall { Update = update }.Write(session.StateFolder);
    Check(session.CheckAsync().Result == null && process.Games == 1 && !session.RepairRequired, "Complete install did not clear its marker.");
  }
  private static void InUseFiles()
  {
    var folder = NewGame(); var path = Path.Combine(folder, "module.dll"); File.WriteAllBytes(path, new byte[] { 1 });
    using (File.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read))
      Check(!InstallFiles.Available(folder), "An open module allowed installation.");
    Check(InstallFiles.Available(folder), "A closed module blocked installation.");
  }
  private static void CorruptManifestOffersRepair()
  {
    foreach (var text in new[] { "{", "null", "{\"version\":\"invalid\"}" })
    {
      var folder = NewGame();
      var first = Session(folder, new FakeProcesses());
      new PendingInstall { Update = Update("https://example.invalid", new byte[] { 1 }) }.Write(first.StateFolder);
      first.Dispose(); File.WriteAllText(Path.Combine(folder, "game", "game.json"), text);
      var process = new FakeProcesses(); var next = Session(folder, process);
      Check(next.CheckAsync().Result?.version == "2.0" && process.Games == 0, "A damaged manifest did not offer repair.");
    }
  }

  private sealed class FakeProcesses : IUpdateProcesses
  {
    public int Games, Helpers, Failures;
    public void StartGame(string folder, string[] args) { Games++; Check(args.SequenceEqual(new[] { "a b" }), "Game arguments changed."); }
    public void StartHelper(string helper, Handoff handoff)
    {
      Helpers++;
      Fails(() => { using (File.Open(handoff.Installer, FileMode.Open, FileAccess.Write, FileShare.ReadWrite)) { } });
    }
    public int Install(string installer, string arguments, string folder, Action<int, long> started)
    {
      throw new InvalidOperationException("A test started an installer.");
    }
    public void ShowStatus(string name, string text)
    {
      Check(Games == 0, "The game started before the status was shown.");
      Failures++;
    }
  }
  private sealed class Server : IDisposable
  {
    private readonly TcpListener _listener = new TcpListener(IPAddress.Loopback, 0);
    public Server() { _listener.Start(); }
    public string Url => "http://127.0.0.1:" + ((IPEndPoint)_listener.LocalEndpoint).Port + "/";
    public Task Respond(byte[] body, int code = 200, bool allowCancel = false, int chunks = 1)
    {
      return Task.Run(async () =>
      {
        var pending = _listener.AcceptTcpClientAsync();
        Check(await Task.WhenAny(pending, Task.Delay(10000)) == pending, "No HTTP request.");
        using (var client = await pending)
        {
          client.ReceiveTimeout = client.SendTimeout = 10000;
          var stream = client.GetStream(); var header = new StringBuilder();
          while (!header.ToString().EndsWith("\r\n\r\n", StringComparison.Ordinal))
          { int ch = stream.ReadByte(); Check(ch >= 0 && header.Length < 65536, "Incomplete request."); header.Append((char)ch); }
          var response = Encoding.ASCII.GetBytes("HTTP/1.1 " + code + " Result\r\nContent-Length: " + body.Length + "\r\nConnection: close\r\n\r\n");
          try
          {
            stream.Write(response, 0, response.Length);
            for (int i = 0; i < chunks; i++)
            {
              int start = body.Length * i / chunks;
              int end = body.Length * (i + 1) / chunks;
              stream.Write(body, start, end - start);
              if (i + 1 < chunks) await Task.Delay(600);
            }
          }
          catch (IOException) when (allowCancel) { }
        }
      });
    }
    public Task Stall(ManualResetEventSlim received, ManualResetEventSlim release)
    {
      return Task.Run(async () => {
        using (var client = await _listener.AcceptTcpClientAsync())
        {
          received.Set();
          Check(release.Wait(10000), "Idle server was not released.");
        }
      });
    }
    public void Dispose() { _listener.Stop(); }
  }
}
