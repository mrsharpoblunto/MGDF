using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;
using File=MGDF.GamesManager.Common.Framework.File;

namespace MGDF.GamesManager.GameSource.Model.FileServers
{
    public class DefaultFileServer: IFileServer
    {
        public void DeleteGameData(string gameDataId, IServerContext serverContext, IRepository repository)
        {
            if (!string.IsNullOrEmpty(gameDataId))
            {
                Guid versionId = new Guid(gameDataId);
                DefaultFileServerGameData data = repository.Get<DefaultFileServerGameData>().SingleOrDefault(d => d.Id == versionId);
                if (data != null)
                {
                    repository.Delete(data);
                }

                if (data != null && !string.IsNullOrEmpty(data.GameFile) && FileSystem.Current.FileExists(serverContext.MapPath(data.GameFile)))
                {
                    FileSystem.Current.GetFile(serverContext.MapPath(data.GameFile)).Delete();
                }
            }
        }

        public string CreateGameData(Developer developer, Game game, GameVersion version, IServerContext serverContext, IRepository repository)
        {
            Guid id = Guid.NewGuid();
            var gameData = new DefaultFileServerGameData
                               {
                                   Id = id,
                                   GameFile = string.Format("{0}/{1}.fragment", Constants.GameFolder, version.Id.Encode())
                               };
            repository.Insert(gameData);

            return id.ToString();
        }

        public bool UploadPart(IHttpRequest request, GameFragment fragment, string partMd5, IServerContext serverContext, IRepository repository)
        {
            Guid versionId = new Guid(fragment.GameDataId);
            DefaultFileServerGameData data = repository.Get<DefaultFileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            byte[] partBuffer = new byte[request.ContentLength];
            using (Stream stream = new MemoryStream(partBuffer))
            {
                byte[] buffer = new byte[Constants.WriteBufferSize];
                int bytesRead = request.Read(buffer, 0, Constants.WriteBufferSize);
                while (bytesRead > 0)
                {
                    stream.Write(buffer, 0, bytesRead);
                    bytesRead = request.Read(buffer, 0, Constants.WriteBufferSize);
                }
            }

            string computedPartMd5 = partBuffer.ComputeMD5();
            if (partMd5 != computedPartMd5) return false;

            IFile fragmentFile = FileSystem.Current.GetFile(serverContext.MapPath(data.GameFile));
            using (var stream = fragmentFile.OpenStreamWithTimeout(FileMode.Append, FileAccess.Write, FileShare.None))
            {
                stream.Write(partBuffer, 0, request.ContentLength);
            }
            return true;
        }

        public string CompleteUpload(GameFragment fragment, IServerContext serverContext, IRepository repository)
        {
            Guid versionId = new Guid(fragment.GameDataId);
            DefaultFileServerGameData data = repository.Get<DefaultFileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            //rename the fragment file to a completed mza archive
            IFile fragmentFile = FileSystem.Current.GetFile(serverContext.MapPath(data.GameFile));
            fragmentFile.MoveTo(Path.ChangeExtension(fragmentFile.FullName, ".mza"));

            data.GameFile = data.GameFile.Replace(".fragment", ".mza");

            return fragment.GameDataId;

        }

        public bool ValidateCompletedFragment(GameFragment fragment, IServerContext context, IRepository repository)
        {
            Guid versionId = new Guid(fragment.GameDataId);
            DefaultFileServerGameData data = repository.Get<DefaultFileServerGameData>().SingleOrDefault(d => d.Id == versionId);

            IFile fragmentFile = FileSystem.Current.GetFile(context.MapPath(data.GameFile));
            return fragment.Md5Hash == fragmentFile.ComputeMD5();
        }

        public void ServeResponse(IHttpResponse response, Game game, GameVersion version, IRepository repository)
        {
            response.ContentType = Common.Constants.Headers.MzaContentType;
            response.AppendHeader(Headers.ContentDisposition, "attachment; filename=\""+game.Name+".mza\"");
            //do nothing, let the request fall through to the iis static file handler.
        }
    }
}