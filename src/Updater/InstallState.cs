using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Web.Script.Serialization;
using Microsoft.Win32.SafeHandles;

namespace MGDF.Updater
{
  public sealed class InstallLock : IDisposable
  {
    [StructLayout(LayoutKind.Sequential)]
    private struct Security { public int Size; public IntPtr Descriptor; public int Inherit; }
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern SafeFileHandle CreateFile(string file, uint access, uint share, ref Security security,
      uint creation, uint attributes, IntPtr template);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern uint GetFinalPathNameByHandle(SafeFileHandle handle, StringBuilder path, uint length, uint flags);
    public static string CanonicalFolder(string folder)
    {
      var security = new Security { Size = Marshal.SizeOf(typeof(Security)) };
      using (var handle = CreateFile(folder, 0, 7, ref security, 3, 0x02000000, IntPtr.Zero))
      {
        if (handle.IsInvalid) throw new Win32Exception();
        var path = new StringBuilder(32768);
        uint size = GetFinalPathNameByHandle(handle, path, (uint)path.Capacity, 0);
        if (size == 0 || size >= path.Capacity) throw new Win32Exception();
        return path.ToString().TrimEnd('\\').ToUpperInvariant();
      }
    }
    public SafeFileHandle Handle { get; }
    private InstallLock(SafeFileHandle handle) { Handle = handle; }
    public static InstallLock TryAcquire(string folder)
    {
      Directory.CreateDirectory(folder);
      var security = new Security { Size = Marshal.SizeOf(typeof(Security)), Inherit = 1 };
      var handle = CreateFile(Path.Combine(folder, "lock"), 0xC0000000, 0, ref security, 4, 0x80, IntPtr.Zero);
      if (!handle.IsInvalid) return new InstallLock(handle);
      int error = Marshal.GetLastWin32Error(); handle.Dispose();
      if (error == 32 || error == 33) return null;
      throw new Win32Exception(error);
    }
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetHandleInformation(SafeFileHandle handle, uint mask, uint flags);
    public void StopInheritance() { if (!SetHandleInformation(Handle, 1, 0)) throw new Win32Exception(); }
    public static InstallLock Inherited(long value)
    {
      var result = new InstallLock(new SafeFileHandle(new IntPtr(value), true));
      result.StopInheritance(); return result;
    }
    public void Dispose() { Handle.Dispose(); }
  }

  public sealed class PendingInstall
  {
    public UpdateInfo Update { get; set; }
    public string GameName { get; set; }
    public int ProcessId { get; set; }
    public long ProcessStart { get; set; }
    public bool Starting { get; set; }
    public bool IsRunning()
    {
      if (Starting) return true;
      if (ProcessId == 0) return false;
      try
      {
        using (var process = Process.GetProcessById(ProcessId))
          return process.StartTime.ToUniversalTime().Ticks == ProcessStart && !process.HasExited;
      }
      catch (ArgumentException) { return false; }
      catch (InvalidOperationException) { return false; }
      catch (Win32Exception) { return true; }
    }
    public static PendingInstall Read(string folder)
    {
      var path = Path.Combine(folder, "pending.json");
      if (!File.Exists(path)) return null;
      return new JavaScriptSerializer().Deserialize<PendingInstall>(File.ReadAllText(path));
    }
    public void Write(string folder)
    {
      Directory.CreateDirectory(folder);
      var path = Path.Combine(folder, "pending.json");
      var next = path + ".next";
      var bytes = Encoding.UTF8.GetBytes(new JavaScriptSerializer().Serialize(this));
      using (var output = new FileStream(next, FileMode.Create, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough))
      { output.Write(bytes, 0, bytes.Length); output.Flush(true); }
      if (File.Exists(path)) File.Replace(next, path, null); else File.Move(next, path);
    }
    public static void Clear(string folder) { File.Delete(Path.Combine(folder, "pending.json")); }
  }

  public static class InstallFiles
  {
    [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
    private static extern int RmStartSession(out uint session, uint flags, StringBuilder key);
    [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
    private static extern int RmRegisterResources(uint session, uint count, string[] files, uint applications,
      IntPtr processes, uint services, IntPtr serviceNames);
    [DllImport("rstrtmgr.dll")]
    private static extern int RmGetList(uint session, out uint needed, ref uint count, IntPtr processes, ref uint reasons);
    [DllImport("rstrtmgr.dll")] private static extern int RmEndSession(uint session);

    public static bool Available(string folder)
    {
      try
      {
        var files = Directory.GetFiles(folder, "*", SearchOption.AllDirectories).Where(file =>
          string.Equals(Path.GetExtension(file), ".exe", StringComparison.OrdinalIgnoreCase) ||
          string.Equals(Path.GetExtension(file), ".dll", StringComparison.OrdinalIgnoreCase)).ToArray();
        foreach (var file in files)
        {
          using (new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.None)) { }
        }
        if (RmStartSession(out uint session, 0, new StringBuilder(33)) != 0) return false;
        try
        {
          if (RmRegisterResources(session, (uint)files.Length, files, 0, IntPtr.Zero, 0, IntPtr.Zero) != 0) return false;
          uint count = 0, reasons = 0;
          return RmGetList(session, out uint needed, ref count, IntPtr.Zero, ref reasons) == 0 && needed == 0;
        }
        finally { RmEndSession(session); }
      }
      catch (IOException) { return false; }
      catch (UnauthorizedAccessException) { return false; }
    }
  }
}
