using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32.SafeHandles;

namespace MGDF.CrashReporter
{
  public static class Program
  {
    [STAThread]
    private static int Main(string[] args)
    {
      return Run(args, (text, title) => MessageBox.Show(text, title, MessageBoxButtons.OK, MessageBoxIcon.Error));
    }

    public static int Run(string[] args, Action<string, string> showError)
    {
      ReporterOptions options = null;
      CrashResult crash = null;
      try
      {
        options = ReporterOptions.Parse(args);
        using (var log = new SafeFileHandle(options.LogHandle, true))
        {
          crash = CrashMonitor.Wait(options);
          if (!crash.Failed || options.HideErrors) return 0;
          MGDF.Tools.ToolForm.EnableStyles();
          using (var form = new ReportForm(options, crash)) Application.Run(form);
          return 0;
        }
      }
      catch (Exception)
      {
        if (options == null || !options.HideErrors)
          showError((options?.GameName ?? "The game") + " stopped unexpectedly.", options?.GameName ?? "Game error");
        return 1;
      }
      finally
      {
        if (crash?.DumpFile != null && options != null && !options.HideErrors)
        {
          try { File.Delete(crash.DumpFile); }
          catch (IOException) { }
          catch (UnauthorizedAccessException) { }
        }
      }
    }
  }
}
