using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Net;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Helpers
{
    public class AvailableVersion
    {
        public Version Version { get; set; }
        public Uri Url { get; set; }
    }

    public class FrameworkUpdateChecker
    {
        public static bool RequiresUpdate(out string updateFile)
        {
            bool requiresUpdate = false;
            updateFile = string.Empty;

            IDirectory dir = FileSystem.Current.GetDirectory(Constants.FrameworkUpdatesDir);
            if (dir.Exists)
            {
                var updates = dir.GetFiles("*.exe");
                if (updates.Count > 0)
                {
                    updates.Sort((a, b) => b.Name.CompareTo(a.Name));

                    var latestVersion = new Version(updates[0].Name.Replace(".exe", string.Empty));
                    var currentVersion = FileSystem.Current.GetFile(Constants.GamesManagerExecutable).AssemblyVersion;

                    //if there is a version present which is greater than the currently installed version, then lets update.
                    if (latestVersion > currentVersion)
                    {
                        requiresUpdate = true;
                        updateFile = updates[0].FullName;
                    }
                }
            }
            return requiresUpdate;
        }

        public static AvailableVersion CheckLatestVersion() 
        {
            AvailableVersion version;
            string frameworkUpdateSite = ConfigurationManager.AppSettings["frameworkUpdateSite"] ?? "http://www.matchstickframework.org";

            try
            {
                XmlDocument document = new XmlDocument();
                using (var responseStream = HttpRequestManager.Current.GetResponseStream(frameworkUpdateSite+"/latestVersion/"+Constants.InterfaceVersion))
                {
                    document.Load(responseStream);

                    version = new AvailableVersion();
                    var versionElements = document.GetElementsByTagName("version");
                    if (versionElements.Count == 0)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, "Unable to read version from "+frameworkUpdateSite+"/latestVersion");
                        version = null;
                    }
                    else
                    {
                        version.Version = new Version(versionElements[0].InnerText);
                    }

                    if (version != null)
                    {
                        var urlElements = document.GetElementsByTagName("url");
                        if (urlElements.Count == 0)
                        {
                            Logger.Current.Write(LogInfoLevel.Error, "Unable to read url from " + frameworkUpdateSite + "/latestVersion");
                        }
                        else
                        {
                            version.Url = new Uri(urlElements[0].InnerText);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to read " + frameworkUpdateSite + "/latestVersion: " + ex);
                version = null;
            }
            return version;
        } 
    }
}
