using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Threading;
using System.Web;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public class DownloadModule : IHttpModule, IMockableHttpHandler
    {
        private HttpContext _context;

        /// <summary>
        /// unit testing interface
        /// </summary>
        /// <param name="request"></param>
        /// <param name="response"></param>
        public void ProcessRequest(IHttpRequest request, IHttpResponse response)
        {
            DownloadManager.ProcessRequest(ServerContext.Current, GameSourceRepository.Current, request, response);
        }

        #region IHttpModule Members

        public void Dispose()
        {
        }

        public void Init(HttpApplication application)
        {
            //handles authentication of download requests
            application.AddOnAuthenticateRequestAsync(BeginAuthenticateRequestHandlerExecute,EndAuthenticateRequestHandlerExecute);
        }

        #endregion

        #region Event handlers

        IAsyncResult BeginAuthenticateRequestHandlerExecute(Object source,
            EventArgs e, AsyncCallback cb, Object state)
        {
            HttpApplication application = source as HttpApplication;
            _context = application.Context;

            AsyncResult ar = new AsyncResult(cb, state);

            // Create a new thread to perform the operation...note we can't
            // use a thread from the thread pool as this defeats the purpose
            // of the asynchronous implementation (we're trying to leave
            // *those* threads alone for ASP.NET to use to service incoming
            // requests).
            Thread worker = new Thread(DownloadAsync);
            worker.Start(ar);

            return ar;
        }

        private void DownloadAsync(Object asyncResult)
        {
            AsyncResult ar = asyncResult as AsyncResult;

            try
            {
                if (_context.Request.Url.PathAndQuery.StartsWith("/downloads/", StringComparison.InvariantCultureIgnoreCase))
                {
                    //the gamesource context resolver won't find anything as httpcontext is null, so we'll just create our own repo for the async request.
                    using (var repository = new GameSourceRepository(Config.Current.ConnectionString))
                    {
                        DownloadManager.ProcessRequest(new ServerContext(_context), repository, new RealHttpRequest(_context.Request), new RealHttpResponse(_context.Response));
                    }
                }
            }
            catch (Exception)
            {
            }
            ar.CompleteCall();
        }

        void EndAuthenticateRequestHandlerExecute(IAsyncResult ar)
        {
            if (_context.Response.StatusCode != (Int32)HttpStatusCode.OK)
            {
                _context.Response.End();
            }
        }

        #endregion
    }
}
