using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;

internal static partial class Program
{
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  private struct StartupInfo
  {
    public int Size;
    public string Reserved, Desktop, Title;
    public int X, Y, Width, Height, Columns, Rows, Fill, Flags;
    public short Show, ReservedSize;
    public IntPtr ReservedData, Input, Output, Error;
  }
  [StructLayout(LayoutKind.Sequential)]
  private struct ProcessInfo { public IntPtr Process, Thread; public int Id, ThreadId; }
  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
  private struct ProcessEntry
  {
    public uint Size, Usage, Id;
    public UIntPtr Heap;
    public uint Module, Threads, ParentId;
    public int Priority;
    public uint Flags;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string Name;
  }
  private delegate bool EnumWindow(IntPtr window, IntPtr data);
  [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
  private static extern IntPtr CreateDesktop(string name, IntPtr device, IntPtr mode, uint flags, uint access, IntPtr security);
  [DllImport("user32.dll")]
  private static extern bool CloseDesktop(IntPtr desktop);
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
  private static extern bool CreateProcess(string application, StringBuilder command, IntPtr processSecurity,
    IntPtr threadSecurity, bool inherit, uint flags, IntPtr environment, string directory,
    ref StartupInfo startup, out ProcessInfo process);
  [DllImport("kernel32.dll")]
  private static extern uint ResumeThread(IntPtr thread);
  [DllImport("kernel32.dll")]
  private static extern bool CloseHandle(IntPtr handle);
  [DllImport("kernel32.dll", SetLastError = true)]
  private static extern SafeFileHandle CreateToolhelp32Snapshot(uint flags, uint process);
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
  private static extern bool Process32First(SafeFileHandle snapshot, ref ProcessEntry entry);
  [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
  private static extern bool Process32Next(SafeFileHandle snapshot, ref ProcessEntry entry);
  [DllImport("user32.dll")]
  private static extern bool EnumDesktopWindows(IntPtr desktop, EnumWindow callback, IntPtr data);
  [DllImport("user32.dll")]
  private static extern bool EnumChildWindows(IntPtr window, EnumWindow callback, IntPtr data);
  [DllImport("user32.dll")]
  private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  private static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
  [DllImport("user32.dll")]
  private static extern bool IsWindowVisible(IntPtr window);
  [DllImport("user32.dll", SetLastError = true)]
  private static extern IntPtr SendMessageTimeout(IntPtr window, uint message, IntPtr first, IntPtr second,
    uint flags, uint timeout, out UIntPtr result);

  private static Process FindReporter(int parent)
  {
    using (var snapshot = CreateToolhelp32Snapshot(2, 0))
    {
      Check(!snapshot.IsInvalid, "Cannot read child processes.");
      var entry = new ProcessEntry { Size = (uint)Marshal.SizeOf(typeof(ProcessEntry)) };
      bool found = Process32First(snapshot, ref entry);
      while (found)
      {
        if (entry.ParentId == parent && string.Equals(entry.Name, "crashreporter.exe", StringComparison.OrdinalIgnoreCase))
          return Process.GetProcessById((int)entry.Id);
        found = Process32Next(snapshot, ref entry);
      }
    }
    return null;
  }

  private static void FullReportChain()
  {
    // This detects a broken handoff between the crash handler, dump writer, form and upload.
    RunReportChain(false);
    RunReportChain(true);
  }

  private static void RunReportChain(bool fatal)
  {
    var root = Path.Combine(_root, Guid.NewGuid().ToString("N"));
    Directory.CreateDirectory(root);
    var desktopName = "MGDF-report-test-" + Guid.NewGuid().ToString("N");
    var desktop = CreateDesktop(desktopName, IntPtr.Zero, IntPtr.Zero, 0, 0xF01FF, IntPtr.Zero);
    Check(desktop != IntPtr.Zero, "Cannot create private test desktop.");
    Process child = null, reporter = null;
    try
    {
      using (var server = new Server())
      {
        File.WriteAllText(Path.Combine(root, "service-url.txt"), server.Url + "crashes");
        var executable = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "core.tests.exe");
        var command = new StringBuilder("\"" + executable + "\" --crash-reporter-child \"" + root + "\" " + (fatal ? "send-fatal" : "send-crash"));
        var startup = new StartupInfo { Size = Marshal.SizeOf(typeof(StartupInfo)), Desktop = desktopName };
        Check(CreateProcess(executable, command, IntPtr.Zero, IntPtr.Zero, false, 0x08000004,
          IntPtr.Zero, AppDomain.CurrentDomain.BaseDirectory, ref startup, out ProcessInfo info), "Cannot start crash child.");
        try
        {
          child = Process.GetProcessById(info.Id);
          Check(ResumeThread(info.Thread) != uint.MaxValue, "Cannot run crash child.");
        }
        finally { CloseHandle(info.Thread); CloseHandle(info.Process); }
        Check(child.WaitForExit(30000), "Crash child did not exit.");
        Check(unchecked((uint)child.ExitCode) == (fatal ? 1U : 0xC0000005U), "Wrong crash child exit.");
        reporter = FindReporter(child.Id);
        Check(reporter != null, "Crash reporter did not start.");
        Check(reporter.WaitForInputIdle(10000), "Crash form did not become ready.");
        IntPtr send = IntPtr.Zero;
        EnumDesktopWindows(desktop, (window, data) => {
          GetWindowThreadProcessId(window, out uint process);
          if (process != reporter.Id || !IsWindowVisible(window)) return true;
          EnumChildWindows(window, (control, unused) => {
            var text = new StringBuilder(256);
            GetWindowText(control, text, text.Capacity);
            if (text.ToString() == "Send report" && IsWindowVisible(control)) send = control;
            return true;
          }, IntPtr.Zero);
          return true;
        }, IntPtr.Zero);
        Check(send != IntPtr.Zero, "Crash form has no Send report action.");
        Check(SendMessageTimeout(send, 0x00F5, IntPtr.Zero, IntPtr.Zero, 2, 10000, out UIntPtr result) != IntPtr.Zero,
          "Send report did not respond.");
        using (var client = server.Accept())
        {
          var request = ReadRequest(client);
          Check(request.Line == "POST /crashes HTTP/1.1" && request.Headers["Content-Type"] == "application/json", "Wrong crash service request.");
          var json = new System.Web.Script.Serialization.JavaScriptSerializer().Deserialize<Dictionary<string, object>>(Encoding.UTF8.GetString(request.Body));
          Check((string)json["gameUid"] == "crash-test" && ((string)json["detail"]).Contains(fatal ? "Renderer: No DirectX 12 device." : "Exit code: 3221225477."), "Missing crash service details.");
          Respond(client, "{\"success\":true,\"uploadUrl\":\"" + server.Url + "upload\"}");
        }
        using (var client = server.Accept())
        {
          var request = ReadRequest(client);
          Check(request.Line == "PUT /upload HTTP/1.1" && request.Headers["Content-Type"] == "application/zip", "Wrong crash upload.");
          using (var zip = new ZipArchive(new MemoryStream(request.Body), ZipArchiveMode.Read))
          {
            Check(zip.Entries.Count == 2, "Wrong crash archive entries.");
            using (var dump = new MemoryStream())
            {
              zip.GetEntry("minidump.dmp").Open().CopyTo(dump);
              CheckDump(dump, fatal, uint.Parse(File.ReadAllText(Path.Combine(root, "crash-thread.txt"))));
            }
            using (var log = new StreamReader(zip.GetEntry("log.txt").Open()))
            {
              var text = log.ReadToEnd();
              Check(text.Contains("System Information") && text.Contains("MGDF Version: 1.0") &&
                text.Contains("OS Architecture:") && text.Contains("Processor Count:") &&
                text.Contains("Crash send test " + Path.GetFileName(root)), "Missing system or run log data.");
              Check(text.Contains(fatal ? "Renderer: No DirectX 12 device." : "WIN32 ERROR:"), "Missing crash handler text.");
            }
          }
          Respond(client, "");
        }
        Check(reporter.WaitForExit(10000) && reporter.ExitCode == 0, "Reporter did not exit after upload.");
        Check(Directory.GetFiles(root, "*.zip", SearchOption.AllDirectories).Length == 0, "Report zip remains.");
        Check(Directory.GetFiles(root, "*.dmp", SearchOption.AllDirectories).Length == 0, "Report dump remains.");
      }
    }
    finally
    {
      if (child != null)
      {
        if (!child.HasExited) { child.Kill(); child.WaitForExit(10000); }
        if (reporter == null) reporter = FindReporter(child.Id);
        child.Dispose();
      }
      if (reporter != null)
      {
        if (!reporter.HasExited) { reporter.Kill(); reporter.WaitForExit(10000); }
        reporter.Dispose();
      }
      CloseDesktop(desktop);
      Directory.Delete(root, true);
    }
  }

  private static void CheckDump(MemoryStream dump, bool fatal, uint crashedThread)
  {
    var reader = new BinaryReader(dump, Encoding.Unicode, true);
    Check(reader.ReadUInt32() == 0x504D444D, "Invalid dump signature.");
    reader.ReadUInt32();
    uint streams = reader.ReadUInt32(), directory = reader.ReadUInt32();
    Check(streams > 0 && streams <= 64, "Invalid dump directory.");
    var locations = new Dictionary<uint, uint>();
    var sizes = new Dictionary<uint, uint>();
    for (uint i = 0; i < streams; i++)
    {
      dump.Position = directory + i * 12;
      uint type = reader.ReadUInt32(), size = reader.ReadUInt32(), offset = reader.ReadUInt32();
      Check(offset + (long)size <= dump.Length, "Dump stream is outside the file.");
      locations[type] = offset;
      sizes[type] = size;
    }
    Check(locations.ContainsKey(3), "Dump has no thread list.");
    dump.Position = locations[3];
    uint threads = reader.ReadUInt32();
    Check(threads > 0 && threads <= 65536, "Invalid dump thread list.");
    bool found = false;
    for (uint i = 0; i < threads; i++)
    {
      dump.Position = locations[3] + 4 + i * 48;
      found |= reader.ReadUInt32() == crashedThread;
    }
    Check(found, "Dump does not contain the crashed thread.");
    if (fatal)
    {
      Check(!locations.ContainsKey(6) && locations.ContainsKey(11), "Fatal dump has invalid exception data.");
      dump.Position = locations[11];
      Check(Encoding.Unicode.GetString(reader.ReadBytes((int)sizes[11])).TrimEnd('\0') == "Renderer: No DirectX 12 device.", "Fatal dump has no error text.");
    }
    else
    {
      Check(locations.ContainsKey(6), "Dump has no exception stream.");
      dump.Position = locations[6];
      Check(reader.ReadUInt32() == crashedThread, "Wrong dump exception thread.");
      reader.ReadUInt32();
      Check(reader.ReadUInt32() == 0xC0000005, "Wrong dump exception code.");
      dump.Position = locations[6] + 24;
      Check(reader.ReadUInt64() != 0, "Dump has no exception address.");
    }
  }
}
