using System;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace MGDF.Updater
{
  public static class UpdateNetwork
  {
    public static void CheckUrl(Uri uri)
    {
      if (uri.Scheme != Uri.UriSchemeHttps && !(uri.Scheme == Uri.UriSchemeHttp &&
          IPAddress.TryParse(uri.Host, out IPAddress address) && IPAddress.IsLoopback(address)))
        throw new InvalidDataException("Updates require HTTPS.");
    }

    private sealed class DownloadResponse : IDisposable
    {
      public HttpWebResponse Response;
      public CancellationTokenRegistration Cancellation;
      public Stream GetResponseStream() { return Response.GetResponseStream(); }
      public long ContentLength => Response.ContentLength;
      public void Dispose() { Response.Dispose(); Cancellation.Dispose(); }
    }

    private static async Task<DownloadResponse> OpenAsync(Uri uri, CancellationToken token)
    {
      for (int redirects = 0; redirects < 6; redirects++)
      {
        CheckUrl(uri);
        var request = (HttpWebRequest)WebRequest.Create(uri);
        request.AllowAutoRedirect = false;
        request.Proxy = null;
        var cancellation = token.Register(request.Abort);
        try
        {
          var response = (HttpWebResponse)await request.GetResponseAsync().ConfigureAwait(false);
          if ((int)response.StatusCode >= 300 && (int)response.StatusCode < 400)
          {
            using (response) uri = new Uri(uri, response.Headers["Location"]);
            cancellation.Dispose();
          }
          else return new DownloadResponse { Response = response, Cancellation = cancellation };
        }
        catch { cancellation.Dispose(); throw; }
      }
      throw new InvalidDataException("Too many update redirects.");
    }

    public static async Task<UpdateInfo> CheckAsync(GameInfo game)
    {
      if (string.IsNullOrEmpty(game.updateService)) return null;
      try
      {
        using (var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5)))
        using (var response = await OpenAsync(new Uri(game.updateService), timeout.Token).ConfigureAwait(false))
        using (var input = response.GetResponseStream())
        using (var output = new MemoryStream())
        {
          await CopyAsync(input, output, 65536, null, response.ContentLength, timeout.Token).ConfigureAwait(false);
          var update = new JavaScriptSerializer().Deserialize<UpdateInfo>(Encoding.UTF8.GetString(output.ToArray()));
          update.Validate();
          return UpdateInfo.VersionNumber(update.version) > UpdateInfo.VersionNumber(game.version) ? update : null;
        }
      }
      catch (Exception) { return null; }
    }

    public static async Task DownloadAsync(UpdateInfo update, string file, IProgress<int> progress, CancellationToken token)
    {
      using (var response = await OpenAsync(new Uri(update.installerUrl), token).ConfigureAwait(false))
      using (var input = response.GetResponseStream())
      using (var output = new FileStream(file, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        await CopyAsync(input, output, 4L * 1024 * 1024 * 1024, progress, response.ContentLength, token).ConfigureAwait(false);
    }

    private static async Task CopyAsync(Stream input, Stream output, long limit, IProgress<int> progress,
      long length, CancellationToken token)
    {
      var buffer = new byte[65536];
      long total = 0;
      int count;
      while ((count = await input.ReadAsync(buffer, 0, buffer.Length, token).ConfigureAwait(false)) != 0)
      {
        total += count;
        if (total > limit) throw new InvalidDataException("Update is too large.");
        await output.WriteAsync(buffer, 0, count, token).ConfigureAwait(false);
        if (length > 0) progress?.Report((int)Math.Min(100, total * 100 / length));
      }
      if (length >= 0 && length != total) throw new EndOfStreamException();
    }
  }
}
