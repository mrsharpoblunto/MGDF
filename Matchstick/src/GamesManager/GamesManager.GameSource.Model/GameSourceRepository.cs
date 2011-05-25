﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Model
{
    partial class GameSourceRepository: IRepository
    {
        public static IRepositoryContext Context
        { 
            get; set;
        }

        public static IRepository Current
        {
            get
            {
                return Context.Context;
            }
        }

        public static void DisposeCurrent()
        {
            if (Context.HasContext) Context.Context.Dispose();
        }

        void IRepository.SubmitChanges()
        {
            SubmitChanges();
        }

        public IQueryable<TEntity> Get<TEntity>() where TEntity : class
        {
            return GetTable<TEntity>().AsQueryable();
        }

        public void Insert<TEntity>(TEntity instance) where TEntity : class
        {
            GetTable<TEntity>().InsertOnSubmit(instance);
        }

        public void InsertAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class
        {
            GetTable<TEntity>().InsertAllOnSubmit(instances);
        }

        public void Delete<TEntity>(TEntity instance) where TEntity : class
        {
            GetTable<TEntity>().DeleteOnSubmit(instance);
        }

        public void DeleteAll<TEntity>(IEnumerable<TEntity> instances) where TEntity : class
        {
            GetTable<TEntity>().DeleteAllOnSubmit(instances);
        }
    }
}