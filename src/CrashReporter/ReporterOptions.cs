using System;
using System.Collections.Generic;
using System.Globalization;

namespace MGDF.CrashReporter
{
  public sealed class ReporterOptions
  {
    public uint ProcessId { get; set; }
    public IntPtr ProcessHandle { get; set; }
    public IntPtr CrashEvent { get; set; }
    public IntPtr DumpEvent { get; set; }
    public IntPtr ReadyEvent { get; set; }
    public IntPtr MappingHandle { get; set; }
    public string GameName { get; set; }
    public string GameUid { get; set; }
    public string GameVersion { get; set; }
    public string FrameworkVersion { get; set; }
    public string IconPath { get; set; }
    public string UserFolder { get; set; }
    public string LogFile { get; set; }
    public string ServiceUrl { get; set; }
    public string PrivacyPolicyUrl { get; set; }
    public bool HideErrors { get; set; }

    public static ReporterOptions Parse(string[] args)
    {
      if (args.Length != 32) throw new ArgumentException("Invalid reporter arguments.");
      var values = new Dictionary<string, string>(StringComparer.Ordinal);
      for (int i = 0; i < args.Length; i += 2) values.Add(args[i], args[i + 1]);
      return new ReporterOptions
      {
        ProcessId = uint.Parse(values["--process-id"], CultureInfo.InvariantCulture),
        ProcessHandle = Handle(values["--process-handle"]),
        CrashEvent = Handle(values["--crash-event"]),
        DumpEvent = Handle(values["--dump-event"]),
        ReadyEvent = Handle(values["--ready-event"]),
        MappingHandle = Handle(values["--mapping-handle"]),
        GameName = values["--game-name"],
        GameUid = values["--game-uid"],
        GameVersion = values["--game-version"],
        FrameworkVersion = values["--framework-version"],
        IconPath = values["--icon"],
        UserFolder = values["--user-folder"],
        LogFile = values["--log-file"],
        ServiceUrl = values["--service"],
        PrivacyPolicyUrl = values["--privacy-policy"],
        HideErrors = bool.Parse(values["--hide-errors"])
      };
    }

    private static IntPtr Handle(string value)
    {
      var handle = new IntPtr(long.Parse(value, CultureInfo.InvariantCulture));
      if (handle == IntPtr.Zero || handle == new IntPtr(-1))
        throw new ArgumentException("Invalid reporter handle.");
      return handle;
    }
  }
}
