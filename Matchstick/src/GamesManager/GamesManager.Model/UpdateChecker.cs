using System;
using System.Collections.Generic;
using System.Configuration;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using Newtonsoft.Json;

namespace MGDF.GamesManager.Model
{
    public class UpdateDownload
    {
        public string Url;
        public string MD5;
        public string Version;
    }

    public class PartialUpdateDownload
    {
        public string Url;
        public string MD5;
        public string FromVersion;
    }

    public class GameUpdate
    {
        public UpdateDownload Latest;
        public PartialUpdateDownload[] UpdateOlderVersions;
    }

    public class UpdateChecker
    {
        public static UpdateDownload CheckForFrameworkUpdate() 
        {
            UpdateDownload result;
            string frameworkUpdateSite = Config.Current.FrameworkUpdateSite ?? "http://www.matchstickframework.org";

            try
            {
                using (var responseStream = HttpRequestManager.Current.GetResponseStream(frameworkUpdateSite+"/downloads/"+Resources.InterfaceVersion+"/latest.json"))
                {
                    using (var reader = new StreamReader(responseStream))
                    {
                        result = JsonConvert.DeserializeObject<UpdateDownload>(reader.ReadToEnd());
                    }
                }

                var currentVersion = FileSystem.Current.GetFile(Resources.GamesManagerExecutable).AssemblyVersion;

                //if the available version isn't newer than what we have already, then don't bother updating.
                if (new Version(result.Version) <= currentVersion)
                {
                    result = null;
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Unable to read " + frameworkUpdateSite + "/downloads/" + Resources.InterfaceVersion + "/latest.json - " + ex);
                result = null;
            }
            return result;
        } 

        public static UpdateDownload CheckForGameUpdate(Game game)
        {
            GameUpdate availableUpdates;

            if (!string.IsNullOrEmpty(game.GameSourceService))
            {
                try
                {
                    using (var responseStream = HttpRequestManager.Current.GetResponseStream(game.GameSourceService + "/downloads/" + Resources.InterfaceVersion + "/" + game.Uid + "/latest.json"))
                    {
                        using (var reader = new StreamReader(responseStream))
                        {
                            availableUpdates = JsonConvert.DeserializeObject<GameUpdate>(reader.ReadToEnd());
                        }
                    }

                    //if the available version isn't newer than what we have already, then don't bother updating.
                    if (new Version(availableUpdates.Latest.Version) > game.Version)
                    {
                        if (availableUpdates.UpdateOlderVersions != null)
                        {
                            //try to find a partial update for this version if possible
                            foreach (var olderVersion in availableUpdates.UpdateOlderVersions)
                            {
                                if (new Version(olderVersion.FromVersion) == game.Version)
                                {
                                    return new UpdateDownload
                                               {
                                                   Url = olderVersion.Url,
                                                   Version = availableUpdates.Latest.Version,
                                                   MD5 = olderVersion.MD5
                                               };
                                }
                            }
                        }

                        //otherwise resort to a full update
                        return availableUpdates.Latest;
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to read " + game.GameSourceService + "/downloads/" + Resources.InterfaceVersion + "/" + game.Uid + "/latest.json - " + ex);
                }
            }
            return null;
        }
    }
}