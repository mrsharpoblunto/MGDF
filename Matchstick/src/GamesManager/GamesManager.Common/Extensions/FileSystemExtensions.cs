using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common.Extensions
{
  public static class FileSystemExtensions
  {
    public const int RetryCount = 8;
    public const int RetryWaitInterval = 250;

    public static void DeleteWithTimeout(this IFile file)
    {
      int retryCount = 0;

      while (retryCount++ < RetryCount)
      {
        try
        {
          if (file.Exists)
          {
            file.Delete();
          }
          return;
        }
        catch (IOException ex)
        {
          if (retryCount == RetryCount)
          {
            throw ex;
          }
          else
          {
            Thread.Sleep(RetryWaitInterval);
          }
        }
      }
    }

    public static void DeleteWithTimeout(this IDirectory directory)
    {
      int retryCount = 0;

      while (retryCount++ < RetryCount)
      {
        try
        {
          if (directory.Exists)
          {
            directory.Delete();
          }
          return;
        }
        catch (IOException ex)
        {
          if (retryCount == RetryCount)
          {
            throw ex;
          }
          else
          {
            Thread.Sleep(RetryWaitInterval);
          }
        }
      }
    }

    public static Stream OpenStreamWithTimeout(this IFile file, FileMode mode, FileAccess access, FileShare share)
    {
      int retryCount = 0;

      while (retryCount++ < RetryCount)
      {
        try
        {
          return file.OpenStream(mode, access, share);
        }
        catch (IOException ex)
        {
          if (retryCount == RetryCount)
          {
            throw ex;
          }
          else
          {
            Thread.Sleep(RetryWaitInterval);
          }
        }
      }
      return null;
    }

    public static Stream OpenStreamWithTimeout(this IFile file, FileMode mode)
    {
      int retryCount = 0;

      while (retryCount++ < RetryCount)
      {
        try
        {
          return file.OpenStream(mode);
        }
        catch (IOException ex)
        {
          if (retryCount == RetryCount)
          {
            throw ex;
          }
          else
          {
            Thread.Sleep(RetryWaitInterval);
          }
        }
      }
      return null;
    }
  }
}
