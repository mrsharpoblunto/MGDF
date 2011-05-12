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
                            var availableVersions = child.GetFiles("*.exe").ToList();
                            availableVersions.Sort((a,b)=>b.Name.CompareTo(a.Name));

                            if (availableVersions.Count > 0)
                            {
                                string mirror = ConfigurationManager.AppSettings["MirrorBaseUrl"];
                                if (!string.IsNullOrEmpty(mirror) && !mirror.StartsWith("http")) mirror += "http://";
                                if (!string.IsNullOrEmpty(mirror) && !mirror.EndsWith("/")) mirror += "/";
                                string filename = (!string.IsNullOrEmpty(mirror) ? mirror : "/Downloads/") + version + "/" + availableVersions[0].Name;
                                _versions.Add(version, new LatestVersion { Version = new Version(_vesionRegex.Match(availableVersions[0].Name).Value), FileName = filename, MD5 = availableVersions[0].ComputeMD5() });
                            }
                        }
                    }
                    _initialized = true;
                }
                Thread.Sleep(600000);
            }
        }

        public static LatestVersion GetLatestVersion(int version)
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
                if (_versions.ContainsKey(version))
                {
                    return _versions[version];
                }
                return null;
            }
        }
    }
}
