using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.ServerCommon;

namespace MatchstickFramework.Web.Models
{
    public class LatestVersion
    {
        public Version Version { get; set; }
        public string FileName { get; set; }
        public string MD5 { get; set; }
    }

    public static class VersionManager
    {
        private static readonly object _lock = new object();
        private static readonly Dictionary<int, LatestVersion> _versions = new Dictionary<int, LatestVersion>();
        private static readonly Dictionary<int, LatestVersion> _sdkVersions = new Dictionary<int, LatestVersion>();

        private static readonly Regex _vesionRegex = new Regex("[0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*",RegexOptions.Compiled);
        private static bool _initialized;

        private static void CheckVersions()
        {
            while (true)
            {
                IDirectory directory = FileSystem.Current.GetDirectory(ServerContext.Current.MapPath("/Downloads"));

                lock (_lock)
                {
                    _versions.Clear();
                    foreach (var child in directory.GetSubDirectories("*"))
                    {
                        int version;
                        if (int.TryParse(child.Name, out version))
                        {
                            var availableSDKVersions = child.GetFiles("*.zip").ToList();
                            availableSDKVersions.Sort((a, b) => b.Name.CompareTo(a.Name));

                            if (availableSDKVersions.Count > 0)
                            {
                                string filename = "/Downloads/" + version + "/" + availableSDKVersions[0].Name;
                                if (!_sdkVersions.ContainsKey(version))
                                {
                                    _sdkVersions.Add(version, new LatestVersion { Version = new Version(_vesionRegex.Match(availableSDKVersions[0].Name).Value), FileName = filename, MD5 = availableSDKVersions[0].ComputeMD5() });
                                }
                                else
                                {
                                    _sdkVersions[version] = new LatestVersion { Version = new Version(_vesionRegex.Match(availableSDKVersions[0].Name).Value), FileName = filename, MD5 = availableSDKVersions[0].ComputeMD5() };
                                }
                            }
                        }
                    }
                    _initialized = true;
                }
                Thread.Sleep(600000);
            }
        }

        public static LatestVersion GetLatestSDKVersion(int version)
        {
            if (!_initialized)
            {
                lock (_lock)
                {
                    if (!_initialized)
                    {
                        var vesionCheckerThread = new Thread(CheckVersions);
                        vesionCheckerThread.Start();
                    }
                }

                while (!_initialized) Thread.Sleep(100);
            }

            lock (_lock)
            {
                if (_sdkVersions.ContainsKey(version))
                {
                    return _sdkVersions[version];
                }
                return null;
            }
        }
    }
}
