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
    class DataLoaderServiceManager
    {
        public static WCFClient<IGameSourceDeveloperService> Client;
    }

    class DataLoaderPresenterBase<T>: PresenterBase<T> where T:IView 
    {
        protected DataLoaderPresenterBase()
        {
        }

        public static void SetDeveloperService(string developerServiceUrl)
        {
            var factory = new ChannelFactory<IGameSourceDeveloperService>(new WebHttpBinding(), new EndpointAddress(developerServiceUrl));
            factory.Endpoint.Behaviors.Add(new WebHttpBehavior());
            DataLoaderServiceManager.Client = new WCFClient<IGameSourceDeveloperService>(factory);
        }

        public static TReturn DeveloperService<TReturn>(Func<IGameSourceDeveloperService, TReturn> code)
        {
            return DataLoaderServiceManager.Client.Use(code);
        }

        protected U NewRequest<U>() where U : AuthenticatedRequestBase, new()
        {
            return RequestBuilder.Build<U>();
        }
    }
}
