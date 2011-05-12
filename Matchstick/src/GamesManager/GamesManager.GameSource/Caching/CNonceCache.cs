using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Model;
using MGDF.GamesManager.GameSource.Model.Configuration;
using MGDF.GamesManager.ServerCommon;

namespace MGDF.GamesManager.GameSource.Caching
{
    //keeps a cache of all non-expired cnonce values so we can ensure that no one is re-using cnonce values on requests
    //as part of a request replay attack.
    public class CNonceCache
    {
        private static readonly CNonceCache _instance = new CNonceCache();
        public static CNonceCache Instance { get { return _instance; }}

        private readonly ReaderWriterLockSlim _lock = new ReaderWriterLockSlim();
        private readonly Dictionary<string, DateTime> _cnonces = new Dictionary<string, DateTime>();
        private DateTime _lastCleanup = DateTime.MinValue;

        private CNonceCache()
        {
        }

        public void Clear()
        {
            _lock.EnterWriteLock();
            try
            {
                _cnonces.Clear();
            }
            finally
            {
                _lock.ExitWriteLock();
            }
        }

        public bool IsUnique(Guid id, int expireTimeMinutes)
        {
            return IsUnique(id, expireTimeMinutes, GameSourceRepository.Current);
        }

        public bool IsUnique(Guid id, int expireTimeMinutes, IRepository repository)
        {
            _lock.EnterUpgradeableReadLock();

            //clean up expired cnonces every 30 secondsz
            if (_lastCleanup.AddSeconds(30)<TimeService.Current.Now)
            {
                _lock.EnterWriteLock();
                try
                {
                    repository.DeleteAll(repository.Get<CNonce>().Where(c => c.Expires <= TimeService.Current.Now));
                    repository.SubmitChanges();
                    _lastCleanup = TimeService.Current.Now;
                    _cnonces.Clear();
                }
                finally
                {
                    _lock.ExitWriteLock();
                }
            }

            try
            {
                if (_cnonces.ContainsKey(id.ToString()))
                {
                    //if this nonce has now expired, it can be considered unique again.
                    return _cnonces[id.ToString()]<=TimeService.Current.Now;
                }
                else
                {
                    _lock.EnterWriteLock();
                    try
                    {
                        CNonce cnonce = repository.Get<CNonce>().SingleOrDefault(c => c.Id == id);
                        //if it exists in the database
                        if (cnonce!=null)
                        {
                            _cnonces.Add(id.ToString(),cnonce.Expires);
                            //if this nonce has now expired, it can be considered unique again.
                            return cnonce.Expires <= TimeService.Current.Now;
                        }
                        else
                        {
                            //create the cnonce and add it to the database to ensure that this cnonce will cannot be re-used for expireTimeMinutes
                            cnonce = new CNonce{Id = id,Expires = TimeService.Current.Now.AddMinutes(expireTimeMinutes) };
                            repository.Insert(cnonce);
                            repository.SubmitChanges();
                            _cnonces.Add(id.ToString(), cnonce.Expires);
                            return true;
                        }
                    }
                    finally
                    {
                        _lock.ExitWriteLock();
                    }
                }
            }
            finally
            {
                _lock.ExitUpgradeableReadLock();
            }
        }
    }
}
