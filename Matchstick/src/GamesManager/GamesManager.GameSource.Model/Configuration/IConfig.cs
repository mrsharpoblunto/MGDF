﻿using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.GameSource.Model.Configuration
{
    public interface IConfig
    {
        string FragmentUploadHandler { get; }
        string ConnectionString { get; }
        string AuthenticationRealm { get; }
        string FeedName { get; }
        string FeedDescription { get; }
        string Name { get; }
        string Description { get; }
        string FeedLink { get; }
        string BaseUrl { get; }
        string FileServerAssembly { get; }
        string FileServerType { get; }
        int MaxUploadPartSize { get; }
    }
}