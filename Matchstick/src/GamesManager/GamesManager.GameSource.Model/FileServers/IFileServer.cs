using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public interface IFileServer
    {
        string CreateGameData(Developer developer, Game game, GameVersion version, IServerContext serverContext,IRepository repository);
        void DeleteGameData(string gameDataId, IServerContext serverContext, IRepository repository);

        bool UploadPart(IHttpRequest request, GameFragment fragment, string partMd5, IServerContext context, IRepository repository);
        string CompleteUpload(GameFragment fragment, IServerContext context, IRepository repository);
        bool ValidateCompletedFragment(GameFragment fragment, IServerContext context, IRepository repository);

        void ServeResponse(IHttpResponse response, Game game, GameVersion version, IRepository repository);
    }
}