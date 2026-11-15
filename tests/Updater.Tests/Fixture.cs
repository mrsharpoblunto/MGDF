using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading;

internal static class Fixture
{
  [StructLayout(LayoutKind.Sequential)]
  private struct ProcessInfo { public IntPtr Reserved, Peb, Reserved2, Reserved3, Id, Parent; }
  [DllImport("ntdll.dll")]
  private static extern int NtQueryInformationProcess(IntPtr process, int kind, out ProcessInfo info, int size, out int length);

  private static int Main(string[] args)
  {
    if (args.Length == 2 && args[0] == "--wait")
    {
      using (var release = EventWaitHandle.OpenExisting(args[1])) return release.WaitOne(15000) ? 0 : 1;
    }
    using (var current = Process.GetCurrentProcess())
    {
      NtQueryInformationProcess(current.Handle, 0, out ProcessInfo info, Marshal.SizeOf(typeof(ProcessInfo)), out int length);
      string parent;
      var log = Environment.GetEnvironmentVariable("MGDF_UPDATER_TEST_LOG");
      var elevated = new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);
      if (Path.GetFileName(current.MainModule.FileName) == "core.exe")
      {
        File.AppendAllText(log, "game|" + info.Parent.ToInt32() + "|" + elevated + "|" + string.Join(";", args) + "|" + info.Parent.ToInt32() + "\n");
        using (var done = EventWaitHandle.OpenExisting(Environment.GetEnvironmentVariable("MGDF_UPDATER_TEST_DONE"))) done.Set();
        return 0;
      }
      using (var process = Process.GetProcessById(info.Parent.ToInt32())) parent = process.MainModule.FileName;
      var folder = Environment.CommandLine.Substring(Environment.CommandLine.LastIndexOf("/D=", StringComparison.Ordinal) + 3);
      var updater = Path.Combine(folder, "updater.exe");
      File.Move(updater, updater + ".old");
      File.Move(updater + ".old", updater);
      bool locked = false;
      try { using (File.Open(current.MainModule.FileName, FileMode.Open, FileAccess.Write, FileShare.ReadWrite)) { } }
      catch (IOException) { locked = true; }
      bool guarded = false;
      try { using (File.Open(Path.Combine(Environment.GetEnvironmentVariable("MGDF_UPDATER_TEST_STATE"), "lock"), FileMode.Open, FileAccess.ReadWrite, FileShare.None)) { } }
      catch (IOException) { guarded = true; }
      File.AppendAllText(log, "install|" + parent + "|" + locked + "|" + folder + "|" + guarded + "|" + info.Parent.ToInt32() + "|" + current.Id + "\n");
      var gate = Environment.GetEnvironmentVariable("MGDF_UPDATER_TEST_INSTALL_GATE");
      if (!string.IsNullOrEmpty(gate))
      {
        using (var ready = EventWaitHandle.OpenExisting(gate + "-ready"))
        using (var release = EventWaitHandle.OpenExisting(gate + "-release"))
        { ready.Set(); if (!release.WaitOne(15000)) return 19; }
      }
      var manifest = Path.Combine(folder, "game", "game.json");
      File.WriteAllText(manifest, File.ReadAllText(manifest).Replace("1.0", "2.0"));
      return args.Contains("--fail") ? 17 : 0;
    }
  }
}
