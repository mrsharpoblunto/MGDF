using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace MGDF.CrashReporter
{
  public sealed class CrashResult
  {
    public bool Failed { get; set; }
    public string DumpFile { get; set; }
    public string Detail { get; set; }
  }

  public static class CrashMonitor
  {
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    private struct DumpException
    {
      public uint ThreadId;
      public IntPtr ExceptionPointers;
      public int ClientPointers;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr MapViewOfFile(IntPtr mapping, uint access, uint high, uint low, UIntPtr size);
    [DllImport("kernel32.dll")]
    private static extern bool UnmapViewOfFile(IntPtr address);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForMultipleObjects(uint count, IntPtr[] handles, bool all, uint timeout);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetEvent(IntPtr handle);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetExitCodeProcess(IntPtr process, out uint code);
    [DllImport("dbghelp.dll", SetLastError = true)]
    private static extern bool MiniDumpWriteDump(IntPtr process, uint processId, SafeFileHandle file,
      uint type, IntPtr exception, IntPtr streams, IntPtr callback);

    public static CrashResult Wait(ReporterOptions options)
    {
      using (var process = new SafeWaitHandle(options.ProcessHandle, true))
      using (var crash = new SafeWaitHandle(options.CrashEvent, true))
      using (var done = new SafeWaitHandle(options.DumpEvent, true))
      using (var ready = new SafeWaitHandle(options.ReadyEvent, true))
      using (var mapping = new SafeFileHandle(options.MappingHandle, true))
      {
        var data = MapViewOfFile(mapping.DangerousGetHandle(), 4, 0, 0, new UIntPtr(16));
        if (data == IntPtr.Zero) throw new Win32Exception();
        try
        {
          if (!SetEvent(ready.DangerousGetHandle())) throw new Win32Exception();
          // A pending crash takes priority when the process also has exited.
          uint result = WaitForMultipleObjects(2, new[] { options.CrashEvent, options.ProcessHandle }, false, uint.MaxValue);
          if (result > 1) throw new Win32Exception();
          var report = new CrashResult();
          if (result == 0)
          {
            report.Failed = true;
            report.Detail = "The game stopped unexpectedly.";
            try
            {
              report.DumpFile = WriteDump(options, data);
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException || ex is Win32Exception)
            {
              report.Detail += " Dump error: " + ex.Message;
            }
            finally
            {
              SetEvent(done.DangerousGetHandle());
            }
            if (WaitForMultipleObjects(1, new[] { options.ProcessHandle }, false, uint.MaxValue) != 0)
              throw new Win32Exception();
          }
          if (!GetExitCodeProcess(options.ProcessHandle, out uint code)) throw new Win32Exception();
          report.Failed |= code != 0;
          report.Detail = (report.Detail ?? "The game stopped unexpectedly.") + " Exit code: " + code + ".";
          return report;
        }
        finally { UnmapViewOfFile(data); }
      }
    }

    private static string WriteDump(ReporterOptions options, IntPtr data)
    {
      var file = Path.Combine(options.UserFolder, "minidump.dmp");
      var exception = new DumpException
      {
        ThreadId = unchecked((uint)Marshal.ReadInt32(data)),
        ExceptionPointers = new IntPtr(Marshal.ReadInt64(data, 8)),
        ClientPointers = 1
      };
      var info = IntPtr.Zero;
      try
      {
        if (exception.ExceptionPointers != IntPtr.Zero)
        {
          info = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(DumpException)));
          Marshal.StructureToPtr(exception, info, false);
        }
        using (var output = new FileStream(file, FileMode.Create, FileAccess.Write, FileShare.None))
        {
          if (!MiniDumpWriteDump(options.ProcessHandle, options.ProcessId, output.SafeFileHandle,
              0, info, IntPtr.Zero, IntPtr.Zero)) throw new Win32Exception();
        }
        return file;
      }
      catch
      {
        try { File.Delete(file); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
        throw;
      }
      finally { if (info != IntPtr.Zero) Marshal.FreeHGlobal(info); }
    }
  }
}
