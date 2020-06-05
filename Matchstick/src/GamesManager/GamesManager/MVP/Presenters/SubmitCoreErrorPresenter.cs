using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using Amazon.S3.Model;
using Amazon.S3.Transfer;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.MVP.Views;

namespace MGDF.GamesManager.MVP.Presenters
{
  class SubmitCoreErrorPresenter
  {
    public static IPresenter Create(Game game, string message, string detail)
    {
      if (!string.IsNullOrEmpty(game.SupportS3Bucket) &&
        !string.IsNullOrEmpty(game.SupportS3BucketAccessKey) &&
        !string.IsNullOrEmpty(game.SupportS3BucketSecretKey))
      {
        return new SubmitCoreErrorS3Presenter(game, detail);
      }
      else
      {
        return new SubmitCoreErrorEmailPresenter(game, message, detail);
      }
    }

    public static string GetLogContent(Game game, string detail)
    {
      StringBuilder sb = new StringBuilder();

      sb.AppendLine("IMPORTANT: In order to help us find out the source of this problem, please attach the following file to this email before sending.");
      sb.AppendLine();
      sb.AppendLine(Resources.GameUserDir + "\\minidump.dmp");
      sb.AppendLine();
      sb.AppendLine("This file contains important debugging information to allow us to better understand what caused the problem you experienced");
      sb.AppendLine();
      sb.AppendLine();
      sb.AppendLine();

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

  class SubmitCoreErrorS3Presenter : PresenterBase<ISubmitErrorS3View>
  {
    private readonly string _detail;
    private readonly Game _game;

    public SubmitCoreErrorS3Presenter(Game game, string detail)
    {
      _game = game;
      _detail = detail;
      View.SendLogOutput += View_SendLogOutput;
    }

    private async void View_SendLogOutput(object sender, EventArgs e)
    {
      View.Sending = true;
      try
      {
        var logId = DateTime.UtcNow.ToString("dd MMMM yyyy hh:mm:ss", CultureInfo.InvariantCulture);

        var file = Path.Combine(Resources.GameUserDir, "minidump.dmp");
        if (FileSystem.Current.FileExists(file))
        {
          var transfer = new TransferUtility(_game.SupportS3BucketAccessKey, _game.SupportS3BucketSecretKey, Amazon.RegionEndpoint.USEast1);
          var uploadRequest = new TransferUtilityUploadRequest
          {
            FilePath = file,
            BucketName = _game.SupportS3Bucket,
            Key = logId + "/minidump.dmp"
          };
          await transfer.UploadAsync(uploadRequest);
        }

        var client = new Amazon.S3.AmazonS3Client(_game.SupportS3BucketAccessKey, _game.SupportS3BucketSecretKey, Amazon.RegionEndpoint.USEast1);
        var putRequest = new PutObjectRequest
        {
          ContentBody = SubmitCoreErrorPresenter.GetLogContent(_game, _detail),
          BucketName = _game.SupportS3Bucket,
          Key = logId + "/log.txt"
        };
        await client.PutObjectAsync(putRequest);
      }
      catch (Exception ex)
      {
        Message.Show("Failed to send error report");
        Logger.Current.Write(ex, "Failed to send error report");
      }
      View.Sending = false;
      CloseView();
    }
  }

  class SubmitCoreErrorEmailPresenter : PresenterBase<ISubmitErrorEmailView>
  {
    private readonly string _detail;
    private readonly Game _game;

    public SubmitCoreErrorEmailPresenter(Game game, string message, string detail)
    {
      _game = game;
      _detail = detail;
      View.Message = message;
      View.SupportEmail = string.IsNullOrEmpty(game.SupportEmail) ? Resources.SupportEmail : game.SupportEmail;
      View.CopyLogOutput += View_CopyLogOutput;
      View.EmailLogOutput += View_EmailLogOutput;
    }

    void View_EmailLogOutput(object sender, EventArgs e)
    {
      try
      {
        Process.Start("mailto:" + (string.IsNullOrEmpty(_game.SupportEmail) ? Resources.SupportEmail : _game.SupportEmail) + "?subject=Core Error Report (" + _game.Uid + ")");
      }
      catch (Exception ex)
      {
        Message.Show("No email client installed");
        Logger.Current.Write(ex, "No program configured to open mailto: links");
      }
    }

    void View_CopyLogOutput(object sender, EventArgs e)
    {
      var runner = new CrossThreadRunner();
      runner.RunInSTA(() => Clipboard.SetText(SubmitCoreErrorPresenter.GetLogContent(_game, _detail)));
    }
  }
}