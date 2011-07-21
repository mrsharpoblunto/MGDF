using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    class DataSource
    {
        public DataSource()
        {
            PendingInserts = new ArrayList();
            PendingDeletes = new ArrayList();
            CurrentData = new ArrayList();
        }
        public ArrayList PendingInserts { get; private set; }
        public ArrayList PendingDeletes { get; private set; }
        public ArrayList CurrentData { get; private set; }
    }

    public abstract class MockRepositoryBase: IRepository
    {
        private readonly Dictionary<Type, object> _data;

        public MockRepositoryBase(IEnumerable<Type> storeableTypes)
        {
            _data = new Dictionary<Type, object>();
            InitDataSources(storeableTypes);
        }

        public void Dispose()
        {
            _data.Clear();
        }

        public IQueryable<TEntity> Get<TEntity>() where TEntity : class
        {
            return GetDataSource<TEntity>().CurrentData.Cast<TEntity>().AsQueryable();
        }

        public void Insert<TEntity>(TEntity instance) where TEntity : class
        {
            GetDataSource<TEntity>().PendingInserts.Add(instance);
        }

        public void InsertAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class
        {
            foreach (var entity in instances)
            {
                GetDataSource<TEntity>().PendingInserts.Add(entity);
            }
        }

        public void Delete<TEntity>(TEntity instance) where TEntity : class
        {
            GetDataSource<TEntity>().PendingDeletes.Add(instance);
        }

        public void DeleteAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class
        {
            foreach (var entity in instances)
            {
                GetDataSource<TEntity>().PendingDeletes.Add(entity);
            }
        }

        public void SubmitChanges()
        {
            foreach (var datasource in _data)
            {
                if (datasource.Value!=null)
                {
                    DataSource source = (DataSource)datasource.Value;
                    foreach (var pendingDelete in source.PendingDeletes)
                    {
                        source.CurrentData.Remove(pendingDelete);
                    }
                    foreach (var pendingInsert in source.PendingInserts)
                    {
                        source.CurrentData.Add(pendingInsert);
                    }
                    source.PendingDeletes.Clear();
                    source.PendingInserts.Clear();
                }
            }
        }

        private void InitDataSources(IEnumerable<Type> types)
        {
            foreach (var type in types)
            {
                _data.Add(type, null);
            }
        }

        private DataSource GetDataSource<T>()
        {
            if (_data.ContainsKey(typeof(T)))
            {
                if (_data[typeof(T)] == null)
                {
                    _data[typeof(T)] = new DataSource();
                }
                return (DataSource)_data[typeof(T)];
            }
            else
            {
                throw new Exception("No datasource of type " + typeof(T).Name + " registered");
            }
        }
    }
}