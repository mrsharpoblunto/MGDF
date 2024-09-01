using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Configuration;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{

  public class LaunchGamePresenter : PresenterBase<IProgressView>
  {
    [StructLayout(LayoutKind.Sequential)]
    public struct DumpData
    {
      public uint processId;
      public uint threadId;
      public IntPtr exceptionPointers;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct MINIDUMP_EXCEPTION_INFORMATION
    {
      public uint ThreadId;
      public IntPtr ExceptionPointers;
      public int ClientPointers;
    }

    [DllImport("dbghelp.dll")]
    static extern bool MiniDumpWriteDump(IntPtr hProcess, uint ProcessId, IntPtr hFile,
        uint DumpType, ref MINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
        IntPtr UserStreamParam, IntPtr CallbackParam);

    [DllImport("kernel32.dll")]
    static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    static extern bool CloseHandle(IntPtr hObject);

    private uint MiniDumpNormal = 0x00000000;
    private uint PROCESS_ALL_ACCESS = 0x001F0FFF;

    private readonly bool _checkForUpdates;
    private Thread _workerThread;
    private string _minidump;

    public LaunchGamePresenter(bool checkForUpdates)
    {
      View.Shown += View_Shown;
      View.Closing += View_Closing;
      View.OnWindowCopyData += View_OnWindowCopyData;

      _checkForUpdates = checkForUpdates;

      try
      {
        if (Game.Current.GameIconData != null)
        {
          View.GameIcon = Image.FromStream(new MemoryStream(Game.Current.GameIconData));
        }
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, "Unexpected error loading game icon data for '" + Game.Current.Uid + "'");
      }

      //only check for updates once a day
      IFile lastUpdate = FileSystem.Current.GetFile(Resources.UserGameLastUpdateFile());
      if (lastUpdate.Exists && lastUpdate.LastWriteTimeUtc.DayOfYear == TimeService.Current.Now.DayOfYear && lastUpdate.LastWriteTimeUtc.Year == TimeService.Current.Now.Year)
      {
        Logger.Current.Write(LogInfoLevel.Info, "Update check skipped...");
        _checkForUpdates = false;
      }
      else
      {
        lastUpdate.WriteText("Last checked for updates on " + TimeService.Current.Now.ToShortDateString());
      }

      View.HideProgress();
    }

    private void View_OnWindowCopyData(object sender, COPYDATASTRUCT e)
    {
      DumpData dump = (DumpData)Marshal.PtrToStructure(e.lpData, typeof(DumpData));
      WriteMiniDump(dump);
    }

    private void WriteMiniDump(DumpData dump)
    {
      IntPtr process = OpenProcess(PROCESS_ALL_ACCESS, false, dump.processId);
      if (process == IntPtr.Zero)
      {
        Logger.Current.Write(LogInfoLevel.Error, "Failed to open process for minidump. Error code: " + Marshal.GetLastWin32Error());
        return;
      }

      try
      {
        string dumpPath = Path.Combine(Resources.GameUserDir, "minidump.dmp");
        using (FileStream fs = new FileStream(dumpPath, FileMode.Create))
        {
          MINIDUMP_EXCEPTION_INFORMATION mei = new MINIDUMP_EXCEPTION_INFORMATION
          {
            ThreadId = dump.threadId,
            ExceptionPointers = dump.exceptionPointers,
            ClientPointers = 1 // dumping another process
          };

          bool result = MiniDumpWriteDump(process, dump.processId, fs.SafeFileHandle.DangerousGetHandle(),
              MiniDumpNormal, ref mei, IntPtr.Zero, IntPtr.Zero);

          if (!result)
          {
            Logger.Current.Write(LogInfoLevel.Error, "Failed to create minidump. Error code: " + Marshal.GetLastWin32Error());
          }
          else
          {
            _minidump = dumpPath;
          }
        }
      }
      catch (Exception ex)
      {
        Logger.Current.Write(LogInfoLevel.Error, "Failed to create minidump. Exception: " + ex.Message);
      }
      finally
      {
        CloseHandle(process);
      }
    }

    void View_Closing(object sender, EventArgs e)
    {
      if (_workerThread != null && _workerThread.ThreadState == System.Threading.ThreadState.Running) _workerThread.Abort();
    }

    void View_Shown(object sender, EventArgs e)
    {
      View.Title = "Launching " + Game.Current.Name;
      View.Details = "Launching " + Game.Current.Name + ", please wait...";
      _workerThread = new Thread(DoWork);
      _workerThread.Start();
    }

    private void DoWork()
    {
      try
      {
        if (_checkForUpdates)
        {
          Logger.Current.Write(LogInfoLevel.Info, "Checking for updates...");
          View.Invoke(() =>
          {
            View.Title = "Checking for updates...";
            View.Details = "Checking for updates...";
          });

          AvailableUpdates availableUpdates = UpdateChecker.CheckForUpdate(Game.Current);

          if ((availableUpdates.Framework != null || availableUpdates.Game != null) && GetUpdatePermission())
          {
            Logger.Current.Write(LogInfoLevel.Info, "Updates found, restarting elevated...");

            if (!UACControl.IsVistaOrHigher() && !UACControl.IsAdmin())
            {
              //pre vista you can't elevate, so we have to bail out with an error here.
              ViewFactory.Current.CreateView<IMessage>().Show("Updating requires administrator access", "Administrator accesss required");
              _workerThread = null;
              View.Invoke(CloseView);
              return;
            }

            UACControl.RestartElevated(Resources.GamesManagerBootArguments(
                availableUpdates.Game != null ? availableUpdates.Game.Url : string.Empty,
                availableUpdates.Game != null ? availableUpdates.Game.Md5 : string.Empty,
                availableUpdates.Framework != null ? availableUpdates.Framework.Url : string.Empty,
                availableUpdates.Framework != null ? availableUpdates.Framework.Md5 : string.Empty));
            return;
          }

        }

        Launch();
      }
      catch (ThreadAbortException)
      {

      }
      catch (Exception ex)
      {
        _workerThread = null;
        View.Invoke(() => Program.ShowUnhandledError(ex));
      }
    }

    private bool GetUpdatePermission()
    {
      GetUpdatePermissionPresenter presenter = null;
      View.Invoke(() =>
      {
        presenter = new GetUpdatePermissionPresenter();
        presenter.ShowView(View);
      });
      return presenter.Update;
    }

    private bool GetStatisticsPermission(GetStatsPermissionEventArgs arg)
    {
      SendStatisticsPresenter presenter = null;
      View.Invoke(() =>
      {
        presenter = new SendStatisticsPresenter(Game.Current);
        presenter.ShowView(View);
      });
      return presenter.UserPermissionGranted;
    }

    public void Launch()
    {

      string args = Resources.CoreBootArguments();
      if (StatisticsSession.GetStatisticsPermission(Game.Current, GetStatisticsPermission))
      {
        args += Resources.StatisticsServiceArguments();
      }
      View.Invoke(() => args += Resources.LauncherArguments(View.WindowHandle));
      ProcessManager.Current.StartProcess(Resources.MGDFExecutable, args, GameExited, Game.Current);
      View.Invoke(() => View.Hide());
    }

    private void GameExited(object context, int exitCode)
    {
      Game game = (Game)context;

      if (exitCode != 0)
      {
        View.Invoke(() =>
        {
          var presenter = SubmitCoreErrorPresenter.Create(game, _minidump, game.Name + " has ended unexpectedly",
                                                                                    "An unhandled exception or fatal MGDF error has occurred");
          presenter?.ShowView(View);
        });
      }

      View.Invoke(() =>
      {
        _workerThread = null;
        View.CloseView();
      });
    }
  }
}