using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.GameSource.Caching
{
    class CacheEntry<T>
    {
        public T Entity;
        public DateTime TimeStamp;
    }

    //stores entities in a threadsafe cache for up to one minute at a time, useful for reducing database load for regularly accessed entities.
    abstract class EntityCache<T,TU> where T:new()
    {
        private readonly ReaderWriterLockSlim _lock = new ReaderWriterLockSlim();
        private readonly Dictionary<string, CacheEntry<T>> _cache = new Dictionary<string, CacheEntry<T>>();

        public T Get(TU cacheRequest)
        {
            _lock.EnterUpgradeableReadLock();
            try
            {

                string id = GetCacheKey(cacheRequest);
                if (!_cache.ContainsKey(id))
                {
                    var newEntry = new CacheEntry<T>();
                    _cache.Add(id, newEntry);
                    return UpdateCacheEntry(cacheRequest, newEntry).Entity;
                }
                else
                {
                    var entry = _cache[id];
                    if (new TimeSpan(TimeService.Current.Now.Ticks - entry.TimeStamp.Ticks).TotalMinutes > 1)
                    {
                        return UpdateCacheEntry(cacheRequest, entry).Entity;
                    }
                    else
                    {
                        return entry.Entity;
                    }
                }


            }
            finally
            {
                _lock.ExitUpgradeableReadLock();
            }
        }

        private CacheEntry<T> UpdateCacheEntry(TU request, CacheEntry<T> entry)
        {
            _lock.EnterWriteLock();
            try
            {
                entry.Entity = CreateCacheEntry(request);
                return entry;
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }

        protected abstract string GetCacheKey(TU request);
        protected abstract T CreateCacheEntry(TU request);

        public void Invalidate()
        {
            _lock.EnterWriteLock();
            try
            {
                _cache.Clear();
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }
    }
}
