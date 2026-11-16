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
    if (args.Length > 0 && args[0] == "--stage-child")
    {
      var session = new UpdateSession(args[1], Path.Combine(args[1], "updater.exe"),
        new[] { "a b", "quote\"value", "ends\\", "" }, new UpdateProcesses());
      Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_STATE", session.StateFolder);
      session.StageAsync(session.CheckAsync().GetAwaiter().GetResult(), null, CancellationToken.None).GetAwaiter().GetResult();
      return 0;
    }
    _bin = AppDomain.CurrentDomain.BaseDirectory;
    _root = Path.Combine(_bin, "updater-tests-" + Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(_root);
    Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
    MGDF.Tools.ToolForm.EnableStyles();
    Action[] tests = { CoreFlags, NoService, ServiceFailure, SameVersion, Later, PromptLayout, DownloadInactivity, DownloadKeepsReceiving, DownloadFailureStartsGame, CancelFormDownload, CancelDownload, HashMismatch,
      LockedInstaller, InstallerSuccess, InstallerFailure, InstallerRefused, FullHandoff, OneUpdater, PendingInstaller, PendingFailure, CompletedMarker, InUseFiles, KilledHelper, NativeImageInUse, CorruptManifestOffersRepair };
    if (args.Contains("--cpu-only")) tests = tests.Where(test => !new[] { "FullHandoff", "KilledHelper", "NativeImageInUse" }.Contains(test.Method.Name)).ToArray();
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
  private static void Later()
  {
    var process = new FakeProcesses(); var session = Session(NewGame(), process);
    using (var form = new UpdateForm(session, Update("https://example.invalid/", new byte[] { 1 }), ""))
    {
      form.ShowInTaskbar = false; form.StartPosition = FormStartPosition.Manual; form.Location = new System.Drawing.Point(-32000, -32000);
      form.Shown += (sender, e) => form.Controls.OfType<FlowLayoutPanel>().Single().Controls.OfType<Button>().Single(b => b.Text == "Later").PerformClick();
      Application.Run(form);
    }
    Check(process.Games == 1 && process.Helpers == 0, "Later did not start only the game.");
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
  private static void DownloadFailureStartsGame() { DownloadForm(false); }
  private static void CancelFormDownload() { DownloadForm(true); }
  private static void DownloadForm(bool cancel)
  {
    using (var server = new Server())
    using (var received = new ManualResetEventSlim())
    using (var release = new ManualResetEventSlim())
    {
      var process = new FakeProcesses(); var session = Session(NewGame(), process);
      var worker = cancel ? server.Stall(received, release) : server.Respond(new byte[] { 1 }, 500);
      using (var form = new UpdateForm(session, Update(server.Url, new byte[] { 1 }), ""))
      {
        form.ShowInTaskbar = false; form.StartPosition = FormStartPosition.Manual;
        form.Location = new System.Drawing.Point(-32000, -32000);
        form.Shown += (sender, e) => {
          var buttons = form.Controls.OfType<FlowLayoutPanel>().Single().Controls.OfType<Button>().ToArray();
          buttons.Single(button => button.Text == "Update now").PerformClick();
          if (cancel)
          {
            Check(received.Wait(10000), "No request before user cancel.");
            buttons.Single(button => button.Text == "Cancel").PerformClick();
          }
        };
        Application.Run(form);
      }
      release.Set(); Complete(worker);
      Check(process.Games == 1 && process.Helpers == 0, "Download did not return to the game.");
      Check(process.Failures == (cancel ? 0 : 1), "Download failure or user cancel gave the wrong status.");
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
  private static void InstallerSuccess() { Finish(0, 0); }
  private static void InstallerFailure() { Finish(17, 1); }
  private static void InstallerRefused() { Finish(1223, 0); }
  private static void Finish(int code, int failures)
  {
    var folder = NewGame(); var bytes = new byte[] { 5 }; var installer = Path.Combine(_root, "installer-" + Guid.NewGuid().ToString("N") + ".exe"); File.WriteAllBytes(installer, bytes);
    var readyName = "Local\\MGDF-tests-" + Guid.NewGuid().ToString("N");
    var releaseName = "Local\\MGDF-tests-" + Guid.NewGuid().ToString("N");
    using (var signal = new EventWaitHandle(false, EventResetMode.ManualReset, readyName))
    using (var release = new EventWaitHandle(false, EventResetMode.ManualReset, releaseName))
    using (var parent = Process.Start(new ProcessStartInfo(Path.Combine(_bin, "updater.fixture.exe"), "--wait " + releaseName)
      { UseShellExecute = false, CreateNoWindow = true }))
    {
      var state = Path.Combine(folder, "state");
      var installLock = InstallLock.TryAcquire(state);
      var inherited = installLock.Handle.DangerousGetHandle().ToInt64();
      installLock.Handle.SetHandleAsInvalid(); installLock.Dispose();
      var process = new FakeProcesses { Code = code };
      var handoff = new Handoff { ParentId = parent.Id, ParentStart = parent.StartTime.ToUniversalTime().Ticks,
        ReadyEvent = readyName, InstallDirectory = folder, Installer = installer, StateFolder = state, LockHandle = inherited,
        Update = Update("https://example.invalid", bytes),
        Sha256 = Update("https://example.invalid", bytes).sha256, InstallerArguments = new[] { "/S", "/D={installDir}" },
        GameArguments = new[] { "a b" }, GameName = "Test" };
      var worker = Task.Run(() => UpdateSession.Finish(handoff, Path.Combine(folder, "updater.exe"), process));
      Check(signal.WaitOne(10000), "Helper did not signal ready.");
      Check(process.Installs == 0, "Installer started before the first updater exited.");
      release.Set(); Complete(worker);
      Check(worker.Result == 0 && process.Games == (code == 17 ? 0 : 1) && process.Failures == failures && process.Installs == 1, "Wrong installer result.");
    }
  }

  private static void FullHandoff()
  {
    using (var server = new Server())
    {
      var bytes = File.ReadAllBytes(Path.Combine(_bin, "updater.fixture.exe"));
      var update = Update(server.Url, bytes);
      var folder = NewGame(server.Url);
      File.Copy(Path.Combine(_bin, "updater.fixture.exe"), Path.Combine(folder, "core.exe"));
      var log = Path.Combine(folder, "flow.txt");
      var eventName = "Local\\MGDF-tests-" + Guid.NewGuid().ToString("N");
      using (var done = new EventWaitHandle(false, EventResetMode.ManualReset, eventName))
      {
        Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_LOG", log);
        Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_DONE", eventName);
        var worker = Task.Run(async () => { await server.Respond(Encoding.UTF8.GetBytes(Json.Serialize(update))); await server.Respond(bytes); });
        using (var child = Process.Start(new ProcessStartInfo(Path.Combine(_bin, "updater.tests.exe"),
          "--stage-child " + UpdateProcesses.Quote(folder)) { UseShellExecute = false, CreateNoWindow = true }))
        {
          Check(done.WaitOne(15000), "Updated game did not start. " + (File.Exists(log) ? File.ReadAllText(log) : "No fixture log."));
          Check(child.WaitForExit(10000) && child.ExitCode == 0, "First updater did not exit.");
        }
        Complete(worker);
        var lines = File.ReadAllLines(log);
        Check(lines.Length == 2 && lines[0].StartsWith("install|") && lines[0].Contains("|True|"), "Installer was not locked.");
        Check(lines[0].Split('|')[4] == "True", "Temporary updater did not inherit the install lock.");
        Check(lines[1].StartsWith("game|") && lines[1].Contains("|False|a b;quote\"value;ends\\;"), "Game token or arguments changed.");
        Check(lines[0].Contains("MGDF-updater") && lines[0].Contains("updater.exe") && lines[1].Split('|')[1] == lines[0].Split('|')[5], "Temp updater did not own the handoff.");
        Check(GameInfo.Load(folder).version == "2.0", "Installer did not update the game.");
        // The game signal precedes the helper's exit. Wait for that process, then check cleanup.
        try
        {
          using (var helper = Process.GetProcessById(int.Parse(lines[1].Split('|')[4])))
            Check(helper.WaitForExit(10000), "Helper did not exit.");
        }
        catch (ArgumentException) { }
        var session = new UpdateSession(folder, Path.Combine(folder, "updater.exe"), new string[0], new FakeProcesses());
        Sessions.Add(session);
        session.CleanOldFiles(); Check(Directory.GetDirectories(session.TempRoot).Length == 0, "Old temp files remain.");
      }
      Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_LOG", null);
      Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_DONE", null);
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

  private static void NativeImageInUse()
  {
    var folder = NewGame(); var exe = Path.Combine(folder, "core.exe");
    File.Copy(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe"), exe);
    using (var process = Process.Start(new ProcessStartInfo(exe, "/d /c pause")
      { UseShellExecute = false, CreateNoWindow = true, RedirectStandardInput = true, RedirectStandardOutput = true }))
    {
      try { Check(!InstallFiles.Available(folder), "A running native image allowed installation."); }
      finally { if (!process.HasExited) process.Kill(); Check(process.WaitForExit(5000), "Native fixture did not exit."); }
    }
  }

  private static void KilledHelper()
  {
    using (var server = new Server())
    {
      var bytes = File.ReadAllBytes(Path.Combine(_bin, "updater.fixture.exe"));
      var update = Update(server.Url, bytes); var folder = NewGame(server.Url);
      string state;
      using (var probe = new UpdateSession(folder, Path.Combine(folder, "updater.exe"), new string[0], new FakeProcesses())) state = probe.StateFolder;
      var log = Path.Combine(folder, "killed-flow.txt"); var gate = "Local\\MGDF-install-" + Guid.NewGuid().ToString("N");
      using (var ready = new EventWaitHandle(false, EventResetMode.ManualReset, gate + "-ready"))
      using (var release = new EventWaitHandle(false, EventResetMode.ManualReset, gate + "-release"))
      using (var recorded = new ManualResetEventSlim())
      using (var watcher = new FileSystemWatcher(state, "pending.json"))
      {
        Action inspect = () => { try { if (PendingInstall.Read(state)?.ProcessId > 0) recorded.Set(); } catch (IOException) { } };
        watcher.Changed += (sender, e) => inspect(); watcher.Created += (sender, e) => inspect(); watcher.Renamed += (sender, e) => inspect();
        watcher.EnableRaisingEvents = true;
        Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_LOG", log);
        Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_INSTALL_GATE", gate);
        var worker = Task.Run(async () => { await server.Respond(Encoding.UTF8.GetBytes(Json.Serialize(update))); await server.Respond(bytes); });
        try
        {
          using (var first = Process.Start(new ProcessStartInfo(Path.Combine(_bin, "updater.tests.exe"), "--stage-child " + UpdateProcesses.Quote(folder))
            { UseShellExecute = false, CreateNoWindow = true }))
          {
            Check(ready.WaitOne(10000), "Installer did not reach its gate."); inspect();
            Check(recorded.Wait(5000), "Installer identity was not recorded.");
            Check(first.WaitForExit(5000) && first.ExitCode == 0, "First updater did not exit.");
            var row = File.ReadAllLines(log).Single().Split('|');
            using (var helper = Process.GetProcessById(int.Parse(row[5])))
            using (var installer = Process.GetProcessById(int.Parse(row[6])))
            {
              helper.Kill(); Check(helper.WaitForExit(5000), "Temporary updater did not stop.");
              var process = new FakeProcesses();
              using (var second = new UpdateSession(folder, Path.Combine(folder, "updater.exe"), new[] { "a b" }, process))
              {
                Check(second.Lock != null, "Killed helper retained its lock.");
                Check(second.CheckAsync().Result == null && process.Games == 0 && process.Failures == 1, "Live installer did not block a new updater.");
              }
              release.Set(); Check(installer.WaitForExit(10000), "Installer did not finish.");
            }
          }
          Complete(worker);
          var complete = new FakeProcesses();
          using (var next = new UpdateSession(folder, Path.Combine(folder, "updater.exe"), new[] { "a b" }, complete))
          {
            // Do not make a new service request after the server's two responses.
            next.Game.updateService = null;
            Check(next.CheckAsync().Result == null && complete.Games == 1 && !next.RepairRequired, "Completed installer did not permit recovery.");
            next.CleanOldFiles();
          }
        }
        finally
        {
          release.Set(); Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_INSTALL_GATE", null);
          Environment.SetEnvironmentVariable("MGDF_UPDATER_TEST_LOG", null);
        }
      }
    }
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
    public int Games, Helpers, Installs, Failures, Code;
    public void StartGame(string folder, string[] args) { Games++; Check(args.SequenceEqual(new[] { "a b" }), "Game arguments changed."); }
    public void StartHelper(string helper, Handoff handoff)
    {
      Helpers++;
      Fails(() => { using (File.Open(handoff.Installer, FileMode.Open, FileAccess.Write, FileShare.ReadWrite)) { } });
    }
    public int Install(string installer, string arguments, string folder, Action<int, long> started)
    {
      Fails(() => { using (File.Open(installer, FileMode.Open, FileAccess.Write, FileShare.ReadWrite)) { } });
      Installs++; Check(arguments == "/S /D=" + folder, "Wrong NSIS folder arguments.");
      if (Code != 1223) using (var process = Process.GetCurrentProcess()) started(process.Id, process.StartTime.ToUniversalTime().Ticks);
      return Code;
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
