using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.Model
{
  public class DependencyInstaller
  {
    public static void Install()
    {
      try
      {
        var dependenciesFolder = FileSystem.Current.GetDirectory(Resources.DependenciesDir);
        if (!dependenciesFolder.Exists) return;
        var deps = new FrameworkDependencies(Resources.DependenciesFile);

        foreach (KeyValuePair<string, string> dep in deps.Dependencies)
        {
          if (dep.Key.Contains(".."))
          {
            Logger.Current.Write(LogInfoLevel.Error, "Cannot install dependencies outside of the framework dependency folder (" + dep.Key + ")");
          }
          else
          {
            Logger.Current.Write(LogInfoLevel.Info, "Installing dependency " + dep.Key + " " + dep.Value + "...");
            var result = ProcessManager.Current.WaitForProcess(Path.Combine(dependenciesFolder.FullName, dep.Key), dep.Value);
            Logger.Current.Write(result == 0 ? LogInfoLevel.Info : LogInfoLevel.Error, "Dependency " + dep.Key + " completed with code " + result);
          }
        }

        dependenciesFolder.DeleteWithTimeout();
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, "Unable to install dependencies");
      }
    }
  }
}
