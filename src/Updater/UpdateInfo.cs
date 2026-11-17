using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Web.Script.Serialization;

namespace MGDF.Updater
{
  public sealed class GameInfo
  {
    public string gameUid { get; set; }
    public string gameName { get; set; }
    public string version { get; set; }
    public string updateService { get; set; }

    public static GameInfo Load(string folder)
    {
      var game = new JavaScriptSerializer().Deserialize<GameInfo>(File.ReadAllText(Path.Combine(folder, "game", "game.json")));
      if (game == null || !Version.TryParse(game.version, out Version version)) throw new InvalidDataException("Invalid game manifest.");
      return game;
    }
  }

  public sealed class UpdateInfo
  {
    public string version { get; set; }
    public string installerUrl { get; set; }
    public string sha256 { get; set; }
    public string[] installerArguments { get; set; }

    public static Version VersionNumber(string text)
    {
      var value = Version.Parse(text);
      return new Version(value.Major, value.Minor, Math.Max(0, value.Build), Math.Max(0, value.Revision));
    }

    public void Validate()
    {
      VersionNumber(version);
      UpdateNetwork.CheckUrl(new Uri(installerUrl));
      if (sha256 == null || sha256.Length != 64 || !sha256.All(Uri.IsHexDigit) ||
          installerArguments == null || installerArguments.Any(value => value == null || value.Contains("\0")))
        throw new InvalidDataException("Invalid update manifest.");
    }
  }

  public sealed class Handoff
  {
    public string InstallDirectory { get; set; }
    public UpdateInfo Update { get; set; }
    public string StateFolder { get; set; }
    public long LockHandle { get; set; }
    public string Installer { get; set; }
    public string Sha256 { get; set; }
    public string[] InstallerArguments { get; set; }
    public string[] GameArguments { get; set; }
    public string GameName { get; set; }
    public int ParentId { get; set; }
    public long ParentStart { get; set; }
    public string ReadyEvent { get; set; }
  }
}
