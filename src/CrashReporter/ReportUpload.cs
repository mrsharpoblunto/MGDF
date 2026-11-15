using System;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace MGDF.CrashReporter
{
  public static class ReportUpload
  {
    public sealed class ServiceResponse
    {
      public bool success { get; set; }
      public string uploadUrl { get; set; }
    }

    public static void WriteZip(Stream output, ReporterOptions options, CrashResult crash)
    {
      using (var zip = new ZipArchive(output, ZipArchiveMode.Create, true))
      {
        if (!string.IsNullOrEmpty(crash.DumpFile))
        {
          var entry = zip.CreateEntry("minidump.dmp", CompressionLevel.Optimal);
          using (var source = File.OpenRead(crash.DumpFile))
          using (var target = entry.Open()) source.CopyTo(target);
        }
        using (var writer = new StreamWriter(zip.CreateEntry("log.txt", CompressionLevel.Optimal).Open(), new UTF8Encoding(false)))
        {
          writer.WriteLine("System Information");
          writer.WriteLine("==================");
          writer.WriteLine("MGDF Version: " + options.FrameworkVersion);
          writer.WriteLine("OS: " + Environment.OSVersion);
          writer.WriteLine("OS Architecture: " + (Environment.Is64BitOperatingSystem ? "64" : "32") + " bit");
          writer.WriteLine("Processor Count: " + Environment.ProcessorCount);
          writer.WriteLine();
          writer.WriteLine("Details");
          writer.WriteLine("=======");
          writer.WriteLine("Game: " + options.GameUid + " " + options.GameVersion);
          writer.WriteLine(crash.Detail);
          writer.WriteLine();
          writer.WriteLine("Log output");
          writer.WriteLine("==========");
          if (File.Exists(options.LogFile))
          {
            using (var input = new StreamReader(new FileStream(options.LogFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite)))
            {
              var buffer = new char[8192];
              int count;
              while ((count = input.Read(buffer, 0, buffer.Length)) != 0) writer.Write(buffer, 0, count);
            }
          }
          else writer.WriteLine("No Log output");
        }
      }
    }

    public static async Task SendAsync(ReporterOptions options, CrashResult crash,
      IProgress<int> progress, CancellationToken cancellation)
    {
      if (options.HideErrors) throw new InvalidOperationException("Reports are disabled.");
      var file = Path.Combine(options.UserFolder, "crash-report-" + Guid.NewGuid().ToString("N") + ".zip");
      try
      {
        cancellation.ThrowIfCancellationRequested();
        using (var output = File.Create(file)) WriteZip(output, options, crash);
        cancellation.ThrowIfCancellationRequested();
        var body = Encoding.UTF8.GetBytes(new JavaScriptSerializer().Serialize(new
        {
          detail = crash.Detail,
          gameUid = options.GameUid,
          gameVersion = options.GameVersion,
          frameworkVersion = options.FrameworkVersion
        }));
        var request = Request(options.ServiceUrl, "POST", "application/json", body.Length);
        string serviceBody;
        using (cancellation.Register(request.Abort))
        {
          using (var stream = await request.GetRequestStreamAsync().ConfigureAwait(false))
            await stream.WriteAsync(body, 0, body.Length, cancellation).ConfigureAwait(false);
          using (var response = (HttpWebResponse)await request.GetResponseAsync().ConfigureAwait(false))
          {
            RequireSuccess(response);
            using (var reader = new StreamReader(response.GetResponseStream()))
            {
              var text = new StringBuilder();
              var buffer = new char[4096];
              int count;
              while ((count = await reader.ReadAsync(buffer, 0, buffer.Length).ConfigureAwait(false)) != 0)
              {
                if (text.Length + count > 65536) throw new InvalidDataException("Invalid service response.");
                text.Append(buffer, 0, count);
              }
              serviceBody = text.ToString();
            }
          }
        }
        var link = new JavaScriptSerializer().Deserialize<ServiceResponse>(serviceBody);
        if (link == null || !link.success || !Uri.TryCreate(link.uploadUrl, UriKind.Absolute, out Uri uri) ||
            (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
          throw new InvalidDataException("The service did not accept the report.");
        cancellation.ThrowIfCancellationRequested();
        using (var source = File.OpenRead(file))
        {
          request = Request(uri.AbsoluteUri, "PUT", "application/zip", source.Length);
          request.AllowWriteStreamBuffering = false;
          using (cancellation.Register(request.Abort))
          {
            using (var target = await request.GetRequestStreamAsync().ConfigureAwait(false))
            {
              var buffer = new byte[81920];
              long sent = 0;
              int count;
              while ((count = await source.ReadAsync(buffer, 0, buffer.Length, cancellation).ConfigureAwait(false)) != 0)
              {
                await target.WriteAsync(buffer, 0, count, cancellation).ConfigureAwait(false);
                sent += count;
                progress?.Report((int)(100 * sent / source.Length));
              }
            }
            using (var response = (HttpWebResponse)await request.GetResponseAsync().ConfigureAwait(false))
              RequireSuccess(response);
          }
        }
      }
      finally
      {
        try { File.Delete(file); }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
      }
    }

    private static HttpWebRequest Request(string url, string method, string contentType, long length)
    {
      var request = (HttpWebRequest)WebRequest.Create(url);
      request.Method = method;
      request.ContentType = contentType;
      request.ContentLength = length;
      request.AllowAutoRedirect = false;
      request.ServicePoint.Expect100Continue = false;
      request.Timeout = 30000;
      request.ReadWriteTimeout = 30000;
      return request;
    }

    private static void RequireSuccess(HttpWebResponse response)
    {
      if ((int)response.StatusCode < 200 || (int)response.StatusCode >= 300)
        throw new IOException("The server did not accept the report.");
    }
  }
}
