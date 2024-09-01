using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Web;
using System.Windows.Forms;
using Amazon.S3.Model;
using Amazon.S3.Transfer;
using ICSharpCode.SharpZipLib.Zip;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;
using File = System.IO.File;
using ThreadState = System.Threading.ThreadState;

namespace MGDF.GamesManager.MVP.Presenters
{

  class SubmitCoreErrorPresenter
  {
    public static IPresenter Create(Game game, string message, string detail)
    {
      if (
        string.IsNullOrEmpty(game.SupportType) ||
        string.IsNullOrEmpty(game.SupportUrl))
      {
        return null;
      }

      if (game.SupportType.Equals(SupportMethod.S3, StringComparison.InvariantCultureIgnoreCase))
      {
        return new SubmitCoreErrorS3Presenter(game, detail);
      }
      else if (
game.SupportType.Equals(SupportMethod.GitHub, StringComparison.InvariantCultureIgnoreCase) ||
game.SupportType.Equals(SupportMethod.Email, StringComparison.InvariantCultureIgnoreCase))
      {
        return new SubmitCoreErrorEmailPresenter(game, message, detail);
      }
      return null;
    }

    public static string GetLogContent(Game game, string detail)
    {
      StringBuilder sb = new StringBuilder();

      sb.AppendLine("System Information");
      sb.AppendLine("==================");
      sb.AppendLine("MGDF Version: " + Assembly.GetExecutingAssembly().GetName().Version);
      sb.AppendLine("OS: " + EnvironmentSettings.Current.OSName);
      sb.AppendLine("OS Architecture: " + EnvironmentSettings.Current.OSArchitecture + " bit");
      sb.AppendLine("RAM: " + EnvironmentSettings.Current.TotalMemory);
      sb.AppendLine("Processor Count: " + EnvironmentSettings.Current.ProcessorCount);
      sb.AppendLine();

      sb.AppendLine("Video Controllers");
      sb.AppendLine("=================");
      //iterate through all the video controllers and output all the relevant info
      //including driver versions etc.
      ManagementObjectSearcher searcher = new ManagementObjectSearcher("select * from Win32_VideoController");
      int controller = 1;
      foreach (ManagementObject obj in searcher.Get())
      {
        sb.AppendLine(string.Format("Video Controller {0}:", controller));
        foreach (PropertyData property in obj.Properties)
        {
          if (property.Value != null)
          {
            sb.AppendLine(property.Name + ": " + property.Value.ToString());
          }
        }
        sb.AppendLine();
        controller++;
      }

      sb.AppendLine("Details");
      sb.AppendLine("=======");
      sb.AppendLine("Game: " + game.Uid + " " + game.Version);
      sb.AppendLine(detail);
      sb.AppendLine();

      sb.AppendLine("Log output");
      sb.AppendLine("==========");
      IFile coreLog = FileSystem.Current.GetFile(Path.Combine(Resources.GameUserDir, "corelog.txt"));
      if (!coreLog.Exists)
      {
        coreLog = FileSystem.Current.GetFile(Path.Combine(EnvironmentSettings.Current.UserDirectory, "corelog.txt"));
      }
      if (coreLog.Exists)
      {
        using (var stream = coreLog.OpenStream(FileMode.Open, FileAccess.Read, FileShare.Read))
        {
          using (TextReader reader = new StreamReader(stream))
          {
            sb.Append(reader.ReadToEnd());
          }
        }
      }
      else
      {
        sb.Append("No Log output");
      }
      return sb.ToString();
    }
  }

  public class CrashReportUploadRequest
  {
    public string Detail;
    public string GameUid;
    public string GameVersion;
    public string FrameworkVersion;
  }

  public class S3PresignedLinkResponse
  {
    public bool Success;
    public string UploadUrl;
  }

  class SubmitCoreErrorS3Presenter : PresenterBase<ISubmitErrorS3View>
  {
    private Thread _workerThread;
    private readonly string _detail;
    private readonly Game _game;

    public SubmitCoreErrorS3Presenter(Game game, string detail)
    {
      _game = game;
      _detail = detail;
      View.SendLogOutput += View_SendLogOutput;
      View.Closed += View_Closed;
    }

    private void View_Closed(object sender, EventArgs e)
    {
      if (_workerThread != null && _workerThread.ThreadState == ThreadState.Running) _workerThread.Abort();
    }

    private void View_SendLogOutput(object sender, EventArgs e)
    {
      View.Sending = true;
      _workerThread = new Thread(DoWork);
      _workerThread.Start();
    }

    private void DoWork()
    {
      var reportFile = Path.GetTempFileName();
      try
      {
        var dumpFile = Path.Combine(Resources.GameUserDir, "minidump.dmp");
        // create the error report zip file
        using (FileStream output = File.Create(reportFile))
        {
          using (ZipOutputStream zipStream = new ZipOutputStream(output))
          {
            zipStream.SetLevel(9);
            if (FileSystem.Current.FileExists(dumpFile))
            {
              var fi = new FileInfo(dumpFile);
              var dumpEntry = new ZipEntry(fi.Name)
              {
                DateTime = fi.LastWriteTime,
                Size = fi.Length
              };
              zipStream.PutNextEntry(dumpEntry);

              using (FileStream fs = new FileStream(dumpFile, FileMode.Open, FileAccess.Read))
              {
                int sourceBytes;
                byte[] buffer = new byte[81920];
                do
                {
                  sourceBytes = fs.Read(buffer, 0, buffer.Length);
                  zipStream.Write(buffer, 0, sourceBytes);
                } while (sourceBytes > 0);
              }
            }

            var logEntry = new ZipEntry("log.txt")
            {
              DateTime = DateTime.Now
            };
            zipStream.PutNextEntry(logEntry);
            byte[] data = Encoding.UTF8.GetBytes(SubmitCoreErrorPresenter.GetLogContent(_game, _detail));
            zipStream.Write(data, 0, data.Length);
          }
        }

        // get a presigned url to upload the error report
        var uploadUrlResponse = HttpRequestManager.Current.PostJson<S3PresignedLinkResponse, CrashReportUploadRequest>(_game.SupportUrl, new CrashReportUploadRequest
        {
          Detail = _detail,
          GameUid = _game.Uid,
          GameVersion = _game.Version.ToString(),
          FrameworkVersion = Assembly.GetExecutingAssembly().GetName().Version.ToString(),
        });

        // upload the error report
        if (uploadUrlResponse.Success)
        {
          using (FileStream uploadStream = File.OpenRead(reportFile))
          {
            HttpRequestManager.Current.Upload(uploadUrlResponse.UploadUrl, uploadStream, "application/zip");
          }
        }
        else
        {
          throw new Exception("Failed to generate a presigned url for the error report upload");

        }
      }
      catch (Exception ex)
      {
        Message.Show("Failed to send error report");
        Logger.Current.Write(ex, "Failed to send error report");
      }
      finally
      {
        try
        {
          FileSystem.Current.GetFile(reportFile).Delete();
        }
        catch { }
        _workerThread = null;
      }

      View.Invoke(() =>
      {
        View.Sending = false;
        CloseView();
      });
    }
  }

  class SubmitCoreErrorEmailPresenter : PresenterBase<ISubmitErrorView>
  {
    private readonly string _detail;
    private readonly Game _game;

    public SubmitCoreErrorEmailPresenter(Game game, string message, string detail)
    {
      _game = game;
      _detail = detail;
      View.Message = message;
      View.SupportType = game.SupportType;
      View.SupportUrl = game.SupportUrl;
      View.CopyLogOutput += View_CopyLogOutput;
      View.SendLogOutput += View_SendLogOutput;
    }

    void View_SendLogOutput(object sender, EventArgs e)
    {
      if (View.SupportType.Equals(SupportMethod.Email, StringComparison.InvariantCultureIgnoreCase))
      {
        try
        {
          Process.Start($"mailto:{View.SupportUrl}?subject={HttpUtility.UrlEncode($"Error Report ({_game.Uid})")}&body={HttpUtility.UrlEncode("IMPORTANT: In order to help us find out the source of this problem, please paste in the details from the error report along with a description of what happened, or the steps to reproduce the problem.")}");
        }
        catch (Exception ex)
        {
          Message.Show("No email client installed");
          Logger.Current.Write(ex, "No program configured to open mailto: links");
        }
      }
      else if (View.SupportType.Equals(SupportMethod.GitHub, StringComparison.InvariantCultureIgnoreCase))
      {
        try
        {
          Process.Start($"{View.SupportUrl}/issues/new?title={HttpUtility.UrlEncode($"Error Report ({_game.Uid})")}&body={HttpUtility.UrlEncode("IMPORTANT: In order to help us find out the source of this problem, please paste in the details from the error report along with a description of what happened, or the steps to reproduce the problem.")}");
        }
        catch (Exception ex)
        {
          Message.Show("No browser installed");
          Logger.Current.Write(ex, "No program configured to open https: links");
        }
      }
    }

    void View_CopyLogOutput(object sender, EventArgs e)
    {
      var sb = new StringBuilder();
      sb.AppendLine("IMPORTANT: In order to help us find out the source of this problem, please also attach the following file to this report.");
      sb.AppendLine();
      sb.AppendLine(Resources.GameUserDir + "\\minidump.dmp");
      sb.AppendLine();
      sb.AppendLine("This file contains important debugging information to allow us to better understand what caused the problem you experienced");
      sb.AppendLine();
      sb.AppendLine();
      sb.AppendLine();
      sb.Append(SubmitCoreErrorPresenter.GetLogContent(_game, _detail));

      var runner = new CrossThreadRunner();
      runner.RunInSTA(() => Clipboard.SetText(sb.ToString()));
    }
  }
}