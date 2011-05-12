using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web;

namespace MGDF.GamesManager.GameSource.Handlers
{
    public class AsyncResult: IAsyncResult
    {
        private AsyncCallback _cb;
        private object _state;
        private ManualResetEvent _event;
        private bool _completed = false;
        private object _lock = new object();

        public AsyncResult(AsyncCallback cb, object state)
        {
            _cb = cb;
            _state = state;
        }
        
        public Object AsyncState { get { return _state; } }

        public bool CompletedSynchronously { get { return false; } }

        public bool IsCompleted { get { return _completed; } }

        public WaitHandle AsyncWaitHandle
        {
            get
            {
                lock (_lock)
                {
                    if (_event == null)
                        _event = new ManualResetEvent(IsCompleted);
                    return _event;
                }
            }
        }

        public void CompleteCall()
        {
            lock (_lock)
            {
                _completed = true;
                if (_event != null) _event.Set();
            }

            if (_cb != null) _cb(this);
        }
    }
}
