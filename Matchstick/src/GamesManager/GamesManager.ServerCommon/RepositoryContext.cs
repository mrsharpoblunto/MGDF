using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.Text;
using System.Web;

namespace MGDF.GamesManager.ServerCommon
{
    public interface IRepository : IDisposable
    {
        IQueryable<TEntity> Get<TEntity>() where TEntity : class;
        void Insert<TEntity>(TEntity instance) where TEntity : class;
        void InsertAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class;
        void Delete<TEntity>(TEntity instance) where TEntity : class;
        void DeleteAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class;
        void SubmitChanges();
    }

    public interface IRepositoryContext
    {
        IRepository Context { get; }
        bool HasContext { get; }
    }

    public class WebRepositoryContext : IRepositoryContext
    {
        private readonly Type _type;
        private readonly Func<IRepository> _repositoryFactory;

        public WebRepositoryContext(Type type,Func<IRepository> createRepository)
        {
            _repositoryFactory = createRepository;
            _type = type;
        }

        public IRepository Context
        {
            get
            {
                if (!HasContext)
                {
                    HttpContext.Current.Items.Add(_type.ToString(), _repositoryFactory());
                }
                return (IRepository)HttpContext.Current.Items[_type.ToString()];
            }
        }

        public bool HasContext
        {
            get { return HttpContext.Current.Items.Contains(_type.ToString()); }
        }
    }

    public class WcfRepositoryContext : IRepositoryContext
    {
        private readonly Type _type;
        private readonly Func<IRepository> _repositoryFactory;

        public WcfRepositoryContext(Type type, Func<IRepository> createRepository)
        {
            _repositoryFactory = createRepository;
            _type = type;
        }

        public IRepository Context
        {
            get
            {
                if (HttpContext.Current != null)
                {
                    if (!HttpContext.Current.Items.Contains(_type.ToString()))
                    {
                        HttpContext.Current.Items.Add(_type.ToString(), _repositoryFactory());
                    }
                    return (IRepository)HttpContext.Current.Items[_type.ToString()];
                }
                else
                {
                    //if we don't have an httpcontext then we are running under WCF, so lazily create a WCF specific context
                    var extension = OperationContext.Current.Extensions.Find<WcfRepositoryContextExtension>();
                    if (extension == null)
                    {
                        extension = new WcfRepositoryContextExtension();
                        OperationContext.Current.Extensions.Add(extension);
                    }

                    //because this extension is shared between multiple contexts, make sure it contains the correct repository
                    if (!extension.ContainsRepository(_type)) extension.CreateRepository(_type, _repositoryFactory());
                    return extension[_type];
                }
            }
        }

        public bool HasContext
        {
            get
            {
                if (HttpContext.Current != null)
                {
                    return HttpContext.Current.Items.Contains(_type.ToString());
                }
                else
                {
                    //if we don't have an httpcontext then we are running under WCF, so lazily create a WCF specific context
                    var extension = OperationContext.Current.Extensions.Find<WcfRepositoryContextExtension>();
                    return extension != null && extension.ContainsRepository(_type);
                }
            }
        }


        private class WcfRepositoryContextExtension : IExtension<OperationContext>
        {
            private readonly Dictionary<Type, IRepository> _repositories;

            public WcfRepositoryContextExtension()
            {
                _repositories = new Dictionary<Type, IRepository>();
            }

            public void Attach(OperationContext owner)
            {
                owner.OperationCompleted += (s, e) => ((OperationContext)s).Extensions.Remove(this);
            }

            public void Detach(OperationContext owner)
            {
                foreach (var kv in _repositories)
                {
                    kv.Value.Dispose();
                }
            }

            public bool ContainsRepository(Type type)
            {
                return _repositories.ContainsKey(type);
            }

            public void CreateRepository(Type type, IRepository repository)
            {
                _repositories.Add(type, repository);
            }

            public IRepository this[Type type]
            {
                get { return _repositories[type]; }
            }
        }
    }

    public class RepositoryContext : IRepositoryContext
    {
        private readonly Func<IRepository> _repositoryFactory;
        private IRepository _repository = null;

        public RepositoryContext(Func<IRepository> createRepository)
        {
            _repositoryFactory = createRepository;
        }

        public IRepository Context 
        { 
            get
            {
                if (_repository==null)
                {
                    _repository = _repositoryFactory();
                }
                return _repository;
            } 
        }

        public bool HasContext
        {
            get { return _repository != null; }
        }
    }
}