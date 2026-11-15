using System;
using System.Windows.Forms;

namespace MGDF.CrashReporter
{
  internal static class Program
  {
    [STAThread]
    private static int Main(string[] args)
    {
      try
      {
        var options = ReporterOptions.Parse(args);
        var crash = CrashMonitor.Wait(options);
        if (!crash.Failed || options.HideErrors) return 0;
        MGDF.Tools.ToolForm.EnableStyles();
        using (var form = new ReportForm(options, crash)) Application.Run(form);
        return 0;
      }
      catch (Exception)
      {
        // The reporter must not open a second error dialog.
        return 1;
      }
    }
  }
}
