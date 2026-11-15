using System;
using System.IO;
using System.Text;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using MGDF.Tools;

namespace MGDF.Updater
{
  internal static class Program
  {
    [STAThread]
    private static int Main(string[] args)
    {
      try
      {
        if (args.Length == 1 && args[0] == "--mgdf-tool-check") return 0;
        if (UpdateProcesses.IsElevated()) return 1;
        System.Net.ServicePointManager.SecurityProtocol = System.Net.SecurityProtocolType.Tls12;
        ToolForm.EnableStyles();
        var updater = Application.ExecutablePath;
        var processes = new UpdateProcesses();
        if (args.Length == 2 && args[0] == "--finish")
          return UpdateSession.Finish(new JavaScriptSerializer().Deserialize<Handoff>(
            Encoding.UTF8.GetString(Convert.FromBase64String(args[1]))), updater, processes);
        var folder = Path.GetDirectoryName(updater);
        using (var session = new UpdateSession(folder, updater, args, processes))
        {
        session.CleanOldFiles();
        var update = session.CheckAsync().GetAwaiter().GetResult();
        if (update != null) using (var form = new UpdateForm(session, update, Path.Combine(folder, "game", "gameIcon.png"))) Application.Run(form);
        }
        return 0;
      }
      catch (Exception) { return 1; }
    }
  }
}
