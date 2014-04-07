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
	public class DependancyInstaller
	{
		public static void Install()
		{
			try
			{
				var dependanciesFolder = FileSystem.Current.GetDirectory(Resources.DependanciesDir);
				if (!dependanciesFolder.Exists) return;
				var deps = new FrameworkDependancies(Resources.DependanciesFile);

				foreach (KeyValuePair<string, string> dep in deps.Dependencies)
				{
					if (dep.Key.Contains(".."))
					{
						Logger.Current.Write(LogInfoLevel.Error, "Cannot install dependancies outside of the framework dependancy folder ("+dep.Key+")");
					}
					else
					{
						Logger.Current.Write(LogInfoLevel.Info, "Installing dependancy " + dep.Key + " " + dep.Value + "...");
						var result = ProcessManager.Current.WaitForProcess(Path.Combine(dependanciesFolder.FullName, dep.Key), dep.Value);
						Logger.Current.Write(result == 0 ? LogInfoLevel.Info : LogInfoLevel.Error, "Dependancy " + dep.Key + " completed with code " + result);
					}
				}

				dependanciesFolder.DeleteWithTimeout();
			}
			catch (Exception ex)
			{
				Logger.Current.Write(ex, "Unable to install dependancies");
			}
		}
	}
}
