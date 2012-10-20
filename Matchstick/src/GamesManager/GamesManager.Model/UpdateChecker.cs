using System;
using System.Collections.Generic;
using System.Configuration;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using Newtonsoft.Json;

namespace MGDF.GamesManager.Model
{
    public class AvailableUpdates
    {
        public UpdateDownload Game;
        public UpdateDownload Framework;
    }

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
        public UpdateDownload Framework;
        public PartialUpdateDownload[] UpdateOlderVersions;
    }

    public class UpdateChecker
    {
        public static AvailableUpdates CheckForUpdate(Game game)
        {
            AvailableUpdates result = new AvailableUpdates();
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

                    var currentVersion = FileSystem.Current.GetFile(Resources.GamesManagerExecutable).AssemblyVersion;

                    //if the available version is specified and is newer than what we have already, then the framework needs an update
                    if (availableUpdates.Framework!=null && new Version(availableUpdates.Framework.Version) > currentVersion)
                    {
                        result.Framework = availableUpdates.Framework;
                    }

                    //if the available version isn't newer than what we have already, then don't bother updating.
                    if (availableUpdates.Latest!=null && new Version(availableUpdates.Latest.Version) > game.Version)
                    {
                        if (availableUpdates.UpdateOlderVersions != null)
                        {
                            //try to find a partial update for this version if possible
                            foreach (var olderVersion in availableUpdates.UpdateOlderVersions)
                            {
                                if (new Version(olderVersion.FromVersion) == game.Version)
                                {
                                    result.Game = new UpdateDownload
                                               {
                                                   Url = olderVersion.Url,
                                                   Version = availableUpdates.Latest.Version,
                                                   MD5 = olderVersion.MD5
                                               };
                                }
                            }
                        }

                        //otherwise resort to a full update
                        if (result.Game==null) result.Game = availableUpdates.Latest;
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to read " + game.GameSourceService + "/downloads/" + Resources.InterfaceVersion + "/" + game.Uid + "/latest.json - " + ex);
                }
            }
            return result;
        }
    }
}