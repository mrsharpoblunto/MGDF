using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model
{
  public delegate void ProcessExited(object sender, int ExitCode);

  public interface IProcessManager
  {
    void StartProcess(string filePath, string args, ProcessExited callback, object context);
    int WaitForProcess(string filePath, string args);
  }

  /// <summary>
  /// runs an external process with the option of passing command line parameters.
  /// </summary>
  public class ProcessManager : IProcessManager
  {
    public static IProcessManager Current
    {
      get
      {
        return ServiceLocator.Current.Get<IProcessManager>();
      }
      set
      {
        ServiceLocator.Current.Register(value);
      }
    }

    public void StartProcess(string filePath, string args, ProcessExited callback, object context)
    {
      try
      {
        var proc = new Process
        {
          EnableRaisingEvents = true
        };
        proc.StartInfo.FileName = filePath;
        proc.StartInfo.Arguments = args;
        proc.StartInfo.UseShellExecute = false;
        proc.Exited += (sender, e) => callback(context, proc.ExitCode);
        proc.Start();
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, "Failed to launch process " + filePath + " " + args);
        callback(context, -1);
      }
    }

    public int WaitForProcess(string filePath, string args)
    {
      try
      {
        var proc = new Process
        {
          EnableRaisingEvents = false
        };
        proc.StartInfo.FileName = filePath;
        proc.StartInfo.Arguments = args;
        proc.StartInfo.UseShellExecute = false;
        proc.Start();
        proc.WaitForExit();
        return proc.ExitCode;
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, "Failed to launch process " + filePath + " " + args);
        return -1;
      }
    }
  }
}