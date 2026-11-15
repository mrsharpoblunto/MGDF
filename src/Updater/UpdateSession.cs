using System;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MGDF.Updater
{
  public sealed class UpdateSession : IDisposable
  {
    private readonly string _folder;
    private readonly string _updater;
    private readonly string[] _arguments;
    private readonly IUpdateProcesses _processes;
    public string TempRoot { get; }
    public GameInfo Game { get; }
    public string StateFolder { get; }
    public InstallLock Lock { get; }
    public bool RepairRequired => PendingInstall.Read(StateFolder) != null;

    public UpdateSession(string folder, string updater, string[] arguments, IUpdateProcesses processes, string temp = null)
    {
      _folder = Path.GetFullPath(folder);
      _updater = updater;
      _arguments = arguments;
      _processes = processes;
      using (var hash = SHA256.Create())
      {
        var id = BitConverter.ToString(hash.ComputeHash(Encoding.UTF8.GetBytes(InstallLock.CanonicalFolder(_folder)))).Replace("-", "");
        TempRoot = Path.Combine(temp ?? Path.GetTempPath(), "MGDF-updater", id);
        StateFolder = Path.Combine(temp ?? Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MGDF-updater-state", id);
      }
      Lock = InstallLock.TryAcquire(StateFolder);
      var pending = Lock == null ? null : PendingInstall.Read(StateFolder);
      Game = new GameInfo { gameName = pending?.GameName ?? Path.GetFileName(_folder), version = "0.0" };
      if (Lock != null && pending?.IsRunning() != true)
      {
        try { Game = GameInfo.Load(folder); }
        catch (Exception) when (pending != null) { }
      }
    }

    public void Dispose() { Lock?.Dispose(); }
    public void StartGame()
    {
      if (Lock != null && !RepairRequired) { Lock.StopInheritance(); _processes.StartGame(_folder, _arguments); }
    }
    public void ShowDownloadFailure()
    {
      _processes.ShowStatus(Game.gameName, "Cannot download the update.");
    }
    public async Task<UpdateInfo> CheckAsync()
    {
      if (Lock == null) { _processes.ShowStatus(Game.gameName, "An update is in progress."); return null; }
      var pending = PendingInstall.Read(StateFolder);
      if (pending != null)
      {
        if (pending.IsRunning()) { _processes.ShowStatus(Game.gameName, "An update is in progress."); return null; }
        if (UpdateInfo.VersionNumber(Game.version) >= UpdateInfo.VersionNumber(pending.Update.version)) PendingInstall.Clear(StateFolder);
      }
      var update = await UpdateNetwork.CheckAsync(Game).ConfigureAwait(false);
      if (RepairRequired) update = update ?? pending.Update;
      if (update == null) StartGame();
      return update;
    }

    public void CleanOldFiles()
    {
      if (Lock == null || PendingInstall.Read(StateFolder)?.IsRunning() == true || !Directory.Exists(TempRoot)) return;
      foreach (var folder in Directory.GetDirectories(TempRoot))
      {
        try
        {
          if ((File.GetAttributes(folder) & FileAttributes.ReparsePoint) != 0) continue;
          using (var lease = new FileStream(Path.Combine(folder, "active"), FileMode.Open, FileAccess.ReadWrite, FileShare.None))
          {
            foreach (var file in Directory.GetFiles(folder))
              if (Path.GetFileName(file) != "active") File.Delete(file);
          }
          File.Delete(Path.Combine(folder, "active"));
          Directory.Delete(folder);
        }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
      }
    }

    public async Task StageAsync(UpdateInfo update, IProgress<int> progress, CancellationToken token)
    {
      if (Lock == null) throw new IOException("An update is in progress.");
      update.Validate();
      var folder = Path.Combine(TempRoot, Guid.NewGuid().ToString("N"));
      Directory.CreateDirectory(folder);
      using (var lease = Lease(folder))
      {
        var installer = Path.Combine(folder, "installer.exe");
        await UpdateNetwork.DownloadAsync(update, installer, progress, token).ConfigureAwait(false);
        using (InstallerVerification.Open(installer, update.sha256, _updater))
        using (var parent = Process.GetCurrentProcess())
        {
          var helper = Path.Combine(folder, "updater.exe");
          File.Copy(_updater, helper);
          token.ThrowIfCancellationRequested();
          _processes.StartHelper(helper, new Handoff
          {
            InstallDirectory = _folder, Installer = installer, Sha256 = update.sha256,
            Update = update, StateFolder = StateFolder, LockHandle = Lock.Handle.DangerousGetHandle().ToInt64(),
            InstallerArguments = update.installerArguments, GameArguments = _arguments, GameName = Game.gameName,
            ParentId = parent.Id, ParentStart = parent.StartTime.ToUniversalTime().Ticks,
            ReadyEvent = "Local\\MGDF-update-" + Guid.NewGuid().ToString("N")
          });
        }
      }
    }

    private static FileStream Lease(string folder)
    {
      return new FileStream(Path.Combine(folder, "active"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite);
    }

    public static int Finish(Handoff handoff, string updater, IUpdateProcesses processes)
    {
      bool ready = false;
      bool started = false;
      var previous = PendingInstall.Read(handoff.StateFolder);
      try
      {
        using (var installLock = InstallLock.Inherited(handoff.LockHandle))
        using (var parent = Process.GetProcessById(handoff.ParentId))
        using (var lease = Lease(Path.GetDirectoryName(updater)))
        using (var installer = InstallerVerification.Open(handoff.Installer, handoff.Sha256, updater))
        using (var signal = EventWaitHandle.OpenExisting(handoff.ReadyEvent))
        {
          if (parent.StartTime.ToUniversalTime().Ticks != handoff.ParentStart) return 1;
          var handle = parent.Handle;
          signal.Set(); ready = true;
          if (!parent.WaitForExit(30000)) return 1;
          if (!InstallFiles.Available(handoff.InstallDirectory))
          {
            processes.ShowStatus(handoff.GameName, "Close the game and its crash report window, then try the update again.");
          }
          else
          {
            var pending = new PendingInstall { Update = handoff.Update, GameName = handoff.GameName, Starting = true };
            pending.Write(handoff.StateFolder);
            int code = processes.Install(handoff.Installer,
              UpdateProcesses.InstallerArguments(handoff.InstallerArguments, handoff.InstallDirectory), handoff.InstallDirectory,
              (pid, time) => { started = true; pending.ProcessId = pid; pending.ProcessStart = time; pending.Starting = false; pending.Write(handoff.StateFolder); });
            if (code == 0 || code == 3010 || code == 1641 ||
                UpdateInfo.VersionNumber(GameInfo.Load(handoff.InstallDirectory).version) >= UpdateInfo.VersionNumber(handoff.Update.version))
              PendingInstall.Clear(handoff.StateFolder);
            else if (!started && previous == null) PendingInstall.Clear(handoff.StateFolder);
            else { pending.ProcessId = 0; pending.Starting = false; pending.Write(handoff.StateFolder); }
            if (code != 0 && code != 3010 && code != 1641 && code != 1223)
              processes.ShowStatus(handoff.GameName, "Cannot install the update. Run the updater again to repair the game.");
          }
          if (PendingInstall.Read(handoff.StateFolder) == null)
            processes.StartGame(handoff.InstallDirectory, handoff.GameArguments);
        }
      }
      catch (Exception)
      {
        if (!ready) return 1;
        processes.ShowStatus(handoff.GameName, "Cannot complete the update. Run the updater again to repair the game.");
      }
      return 0;
    }
  }
}
