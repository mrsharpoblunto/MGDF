using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using Microsoft.Win32.SafeHandles;

namespace MGDF.Updater
{
  public interface IUpdateProcesses
  {
    void StartGame(string folder, string[] arguments);
    void StartHelper(string helper, Handoff handoff);
    int Install(string installer, string arguments, string folder, Action<int, long> started);
    void ShowStatus(string gameName, string text);
  }

  public sealed class UpdateProcesses : IUpdateProcesses
  {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct ShellInfo
    {
      public int Size;
      public uint Mask;
      public IntPtr Window;
      public string Verb, File, Parameters, Directory;
      public int Show;
      public IntPtr Instance, IdList;
      public string Class;
      public IntPtr ClassKey;
      public uint HotKey;
      public IntPtr Icon, Process;
    }
    [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool ShellExecuteEx(ref ShellInfo info);
    [DllImport("kernel32.dll")] private static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll")] private static extern uint WaitForSingleObject(IntPtr handle, uint timeout);
    [DllImport("kernel32.dll")] private static extern bool GetExitCodeProcess(IntPtr process, out int code);
    [DllImport("advapi32.dll", SetLastError = true)]
    private static extern bool OpenProcessToken(IntPtr process, uint access, out SafeFileHandle token);
    [DllImport("advapi32.dll", SetLastError = true)]
    private static extern bool GetTokenInformation(SafeFileHandle token, int kind, out int value, int size, out int length);

    public static bool IsElevated()
    {
      using (var process = Process.GetCurrentProcess())
      {
        if (!OpenProcessToken(process.Handle, 8, out SafeFileHandle token)) throw new Win32Exception();
        using (token)
        {
          if (!GetTokenInformation(token, 20, out int value, 4, out int length)) throw new Win32Exception();
          return value != 0;
        }
      }
    }

    public static string Quote(string value)
    {
      if (value.Length != 0 && !value.Any(char.IsWhiteSpace) && !value.Contains("\"")) return value;
      var result = new StringBuilder("\"");
      int slashes = 0;
      foreach (char ch in value)
      {
        if (ch == '\\') { slashes++; continue; }
        result.Append('\\', slashes * (ch == '"' ? 2 : 1));
        slashes = 0;
        if (ch == '"') result.Append('\\');
        result.Append(ch);
      }
      return result.Append('\\', slashes * 2).Append('"').ToString();
    }

    public static string InstallerArguments(string[] arguments, string folder)
    {
      return string.Join(" ", arguments.Select((value, index) =>
      {
        value = value.Replace("{installDir}", folder);
        // NSIS reads the unquoted /D value through the end of the command line.
        if (value.StartsWith("/D=", StringComparison.Ordinal))
        {
          if (index != arguments.Length - 1 || value.Contains("\"")) throw new ArgumentException("Invalid install folder argument.");
          return value;
        }
        return Quote(value);
      }));
    }

    public void StartGame(string folder, string[] arguments)
    {
      if (IsElevated()) throw new InvalidOperationException("The game launcher must not run as administrator.");
      using (Process.Start(new ProcessStartInfo(Path.Combine(folder, "core.exe"), string.Join(" ", arguments.Select(Quote)))
      { WorkingDirectory = folder, UseShellExecute = false })) { }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct StartupInfo
    {
      public int Size;
      public IntPtr Reserved, Desktop, Title;
      public int X, Y, Width, Height, XChars, YChars, Fill, Flags;
      public short Show, ReservedSize;
      public IntPtr ReservedData, Input, Output, Error;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct StartupEx { public StartupInfo Startup; public IntPtr Attributes; }
    [StructLayout(LayoutKind.Sequential)]
    private struct ChildInfo { public IntPtr Process, Thread; public int ProcessId, ThreadId; }
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool InitializeProcThreadAttributeList(IntPtr list, int count, int flags, ref IntPtr size);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool UpdateProcThreadAttribute(IntPtr list, uint flags, IntPtr kind, IntPtr value,
      IntPtr size, IntPtr previous, IntPtr returned);
    [DllImport("kernel32.dll")] private static extern void DeleteProcThreadAttributeList(IntPtr list);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool CreateProcess(string file, StringBuilder command, IntPtr processSecurity,
      IntPtr threadSecurity, bool inherit, uint flags, IntPtr environment, string directory,
      ref StartupEx startup, out ChildInfo child);
    [DllImport("kernel32.dll")] private static extern bool TerminateProcess(IntPtr process, int code);
    [DllImport("kernel32.dll")]
    private static extern uint WaitForMultipleObjects(int count, IntPtr[] handles, bool all, uint timeout);

    public void StartHelper(string helper, Handoff handoff)
    {
      using (var ready = new EventWaitHandle(false, EventResetMode.ManualReset, handoff.ReadyEvent))
      {
        var size = IntPtr.Zero;
        InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref size);
        var attributes = Marshal.AllocHGlobal(size);
        var handles = Marshal.AllocHGlobal(IntPtr.Size);
        bool initialized = false;
        try
        {
          initialized = InitializeProcThreadAttributeList(attributes, 1, 0, ref size);
          if (!initialized) throw new Win32Exception();
          Marshal.WriteIntPtr(handles, new IntPtr(handoff.LockHandle));
          if (!UpdateProcThreadAttribute(attributes, 0, new IntPtr(0x20002), handles,
              new IntPtr(IntPtr.Size), IntPtr.Zero, IntPtr.Zero)) throw new Win32Exception();
          var startup = new StartupEx { Startup = new StartupInfo { Size = Marshal.SizeOf(typeof(StartupEx)) }, Attributes = attributes };
          var command = new StringBuilder(Quote(helper) + " --finish " +
            Convert.ToBase64String(Encoding.UTF8.GetBytes(new JavaScriptSerializer().Serialize(handoff))));
          if (!CreateProcess(helper, command, IntPtr.Zero, IntPtr.Zero, true, 0x80000 | 0x08000000,
              IntPtr.Zero, Path.GetDirectoryName(helper), ref startup, out ChildInfo child)) throw new Win32Exception();
          try
          {
            if (WaitForMultipleObjects(2, new[] { ready.SafeWaitHandle.DangerousGetHandle(), child.Process }, false, 15000) == 0) return;
            TerminateProcess(child.Process, 1); WaitForSingleObject(child.Process, 5000);
            throw new IOException("Cannot start the temporary updater.");
          }
          finally { CloseHandle(child.Thread); CloseHandle(child.Process); }
        }
        finally
        {
          if (initialized) DeleteProcThreadAttributeList(attributes);
          Marshal.FreeHGlobal(attributes); Marshal.FreeHGlobal(handles);
        }
      }
    }

    public int Install(string installer, string arguments, string folder, Action<int, long> started)
    {
      var info = new ShellInfo { Size = Marshal.SizeOf(typeof(ShellInfo)), Mask = 0x40 | 0x100 | 0x400,
        File = installer, Parameters = arguments, Directory = folder, Show = 1 };
      if (!ShellExecuteEx(ref info))
      {
        int error = Marshal.GetLastWin32Error();
        if (error == 1223) return 1223;
        throw new Win32Exception(error);
      }
      try
      {
        if (info.Process == IntPtr.Zero) throw new Win32Exception();
        if (!GetProcessTimes(info.Process, out long creation, out long exit, out long kernel, out long user)) throw new Win32Exception();
        started((int)GetProcessId(info.Process), DateTime.FromFileTimeUtc(creation).Ticks);
        if (WaitForSingleObject(info.Process, uint.MaxValue) != 0 ||
            !GetExitCodeProcess(info.Process, out int code)) throw new Win32Exception();
        return code;
      }
      finally { if (info.Process != IntPtr.Zero) CloseHandle(info.Process); }
    }

    [DllImport("kernel32.dll")] private static extern uint GetProcessId(IntPtr process);
    [DllImport("kernel32.dll")] private static extern bool GetProcessTimes(IntPtr process, out long creation, out long exit, out long kernel, out long user);

    public void ShowStatus(string gameName, string text)
    {
      System.Windows.Forms.MessageBox.Show(text, gameName,
        System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning);
    }
  }
}
