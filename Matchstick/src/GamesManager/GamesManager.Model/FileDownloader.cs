using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model
{
  public class FileDownloader : LongRunningTask
  {
    private const int DownloadBufferSize = 32768;

    private readonly string _sourceUrl;
    private readonly string _destinationFileName;
    private readonly string _expectedMd5;
    protected Func<GetCredentialsEventArgs, bool> _getCredentials;
    private bool _cancelPending;

    public FileDownloader(string sourceUrl, string destinationFileName, string expectedMd5, Func<GetCredentialsEventArgs, bool> getCredentials)
    {
      _sourceUrl = sourceUrl;
      _destinationFileName = destinationFileName;
      _expectedMd5 = expectedMd5;
      _getCredentials = getCredentials;
      _cancelPending = false;
    }

    public override LongRunningTaskResult Start()
    {
      LongRunningTaskResult result = LongRunningTaskResult.Completed;
      try
      {
        ServicePointManager.ServerCertificateValidationCallback += OnCheckRemoteCallback;

        long total;
        using (Stream responseSteam = HttpRequestManager.Current.GetResponseStream(_sourceUrl, 0, _getCredentials, out total))
        {
          uint uTotal = total > uint.MaxValue ? uint.MaxValue : (uint)total;
          Total = uTotal;
          using (Stream saveFileStream = FileSystem.Current.GetFile(_destinationFileName).OpenStream(FileMode.Create, FileAccess.Write, FileShare.ReadWrite))
          {
            int bytesRead;
            byte[] downloadBuffer = new byte[DownloadBufferSize];

            while ((bytesRead = responseSteam.Read(downloadBuffer, 0, downloadBuffer.Length)) > 0)
            {
              saveFileStream.Write(downloadBuffer, 0, bytesRead);

              Progress = Progress + (uint)bytesRead;
              if (_cancelPending)
              {
                result = LongRunningTaskResult.Cancelled;
              }
            }
          }
        }

        if (result != LongRunningTaskResult.Cancelled)
        {
          string actualMd5 = FileSystem.Current.GetFile(_destinationFileName).ComputeMD5();
          if (_expectedMd5 != actualMd5)
          {
            Logger.Current.Write(LogInfoLevel.Error, string.Format("Downloaded file {0} MD5 file hash {1} does not match the expected value {2}", _destinationFileName, actualMd5, _expectedMd5));
            result = LongRunningTaskResult.Error;
          }
        }
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, string.Format("Unable to download {0} to {1}", _sourceUrl, _destinationFileName));
        result = LongRunningTaskResult.Error;
      }

      if (result == LongRunningTaskResult.Cancelled)
      {
        try
        {
          FileSystem.Current.GetFile(_destinationFileName).DeleteWithTimeout();
        }
        catch (Exception ex)
        {
          Logger.Current.Write(ex, string.Format("Unable to delete partially downloaded file {0}", _destinationFileName));
        }
      }

      return result;
    }

    public void Cancel()
    {
      _cancelPending = true;
    }

    private static bool OnCheckRemoteCallback(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
    {
      return sslPolicyErrors == SslPolicyErrors.None;
    }
  }
}