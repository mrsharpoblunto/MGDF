using System;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public class UploadHandler : IHttpAsyncHandler, IMockableHttpHandler
    {
        /// <summary>
        /// Synchronous implementation
        /// </summary>
        /// <param name="context"></param>
        public void ProcessRequest(HttpContext context)
        {
            UploadManager.ProcessRequest(ServerContext.Current, GameSourceRepository.Current, new RealHttpRequest(context.Request), new RealHttpResponse(context.Response));
        }

        /// <summary>
        /// unit testing interface
        /// </summary>
        /// <param name="request"></param>
        /// <param name="response"></param>
        public void ProcessRequest(IHttpRequest request, IHttpResponse response)
        {
            UploadManager.ProcessRequest(ServerContext.Current, GameSourceRepository.Current, request, response);
        }

        public bool IsReusable
        {
            get { return false; }
        }

        private HttpContext _context;

        /// <summary>
        /// Async implementation
        /// </summary>
        /// <param name="context"></param>
        /// <param name="cb"></param>
        /// <param name="extraData"></param>
        /// <returns></returns>
        public IAsyncResult BeginProcessRequest(HttpContext context, AsyncCallback cb, Object extraData)
        {
            _context = context;

            AsyncResult ar = new AsyncResult(cb, extraData);

            // Create a new thread to perform the operation...note we can't
            // use a thread from the thread pool as this defeats the purpose
            // of the asynchronous implementation (we're trying to leave
            // *those* threads alone for ASP.NET to use to service incoming
            // requests).
            Thread worker = new Thread(UploadAsync);
            worker.Start(ar);

            return ar;
        }

        private void UploadAsync(Object asyncResult)
        {
            AsyncResult ar = asyncResult as AsyncResult;

            try
            {
                //the gamesource context resolver won't find anything as httpcontext is null, so we'll just create our own repo for the async request.
                using (var repository = new GameSourceRepository(Config.Current.ConnectionString))
                {
                    UploadManager.ProcessRequest(new ServerContext(_context), repository, new RealHttpRequest(_context.Request), new RealHttpResponse(_context.Response));
                }
            }
            catch (Exception)
            {
            }
            ar.CompleteCall();
        }

        public void EndProcessRequest(IAsyncResult result)
        {
        }
    }
}
