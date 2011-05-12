using System;
using System.Collections.Generic;
using System.Linq;
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
        protected DataLoaderPresenterBase()
        {
        }

        protected IGameSourceDeveloperService DeveloperService
        {
            get { return ServiceLocator.Current.Get<IGameSourceDeveloperService>(); }
        }

        protected IGameSourceService GamesService
        {
            get { return ServiceLocator.Current.Get<IGameSourceService>(); }
        }

        protected U NewRequest<U>() where U : AuthenticatedRequestBase, new()
        {
            return RequestBuilder.Build<U>();
        }
    }
}
