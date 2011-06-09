using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.ServiceModel.Description;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter
{
    class DataLoaderPresenterBase<T>: PresenterBase<T> where T:IView 
    {
        private static WCFClient<IGameSourceDeveloperService> _client;

        protected DataLoaderPresenterBase()
        {
        }

        public static void SetDeveloperService(string developerServiceUrl)
        {
            var factory = new ChannelFactory<IGameSourceDeveloperService>(new WebHttpBinding(), new EndpointAddress(developerServiceUrl));
            factory.Endpoint.Behaviors.Add(new WebHttpBehavior());
            _client = new WCFClient<IGameSourceDeveloperService>(factory);
        }

        public static TReturn DeveloperService<TReturn>(Func<IGameSourceDeveloperService, TReturn> code)
        {
            return _client.Use(code);
        }

        protected U NewRequest<U>() where U : AuthenticatedRequestBase, new()
        {
            return RequestBuilder.Build<U>();
        }
    }
}
