using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Events;

namespace MGDF.GamesManager.Model.ServiceModel
{
    class PendingOperationRunner
    {
        public event EventHandler OnComplete;
        public event EventHandler<ReloadGameEventArgs> OnReload;
        public event EventHandler OnError;

        private readonly string _pendingOperationQueueFile;
        private readonly ReaderWriterLockSlim _readerWriterLock;
        private List<PendingOperation> _pendingOperationProxies;
        private readonly List<IPendingOperation> _pendingOperations;
        private IPendingOperation _currentPendingOperation;

        public PendingOperationRunner(string gameUid,ReaderWriterLockSlim readerWriterLock, params IPendingOperation[] pendingOperations)
        {
            _readerWriterLock = readerWriterLock;
            _pendingOperations = pendingOperations.ToList();
            _pendingOperationQueueFile = Constants.PendingOperationQueueFile(gameUid);

            //save the list of pending operatiosn before we begin so we can resume if we get interrupted.
            CreateOperationProxies();
            SetCurrentOperation();
            Save();
        }

        private void CreateOperationProxies()
        {
            _pendingOperationProxies = new List<PendingOperation>();
            foreach (var operation in _pendingOperations)
            {
                _pendingOperationProxies.Add(new PendingOperation
                {
                    Name = operation.Name,
                    Progress = operation.Progress,
                    Total = operation.Total,
                    Status = operation.Status
                });
            }
        }

        private void Save()
        {
            try
            {
                var queue = new PendingOperationQueue(_pendingOperations);
                queue.Save(_pendingOperationQueueFile);
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unable to save pending operations");
            }
        }

        public PendingOperationRunner(string pendingOperationQueueFile, ReaderWriterLockSlim readerWriterLock)
        {
            _pendingOperationQueueFile = pendingOperationQueueFile;
            _readerWriterLock = readerWriterLock;

            try
            {
                var queue = new PendingOperationQueue(_pendingOperationQueueFile, _readerWriterLock);
                _pendingOperations = queue.PendingOperations.ToList();
            }
            catch (Exception ex)
            {
                _pendingOperations = new List<IPendingOperation>();
                Logger.Current.Write(ex,"Unable to load pending operations");
            }

            CreateOperationProxies();
            SetCurrentOperation();
        }

        private void SetCurrentOperation()
        {
            if (_currentPendingOperation != null)
            {
                _currentPendingOperation.OnProgressChange -= CurrentPendingOperation_OnProgressChange;
                _currentPendingOperation.OnStatusChange -= CurrentPendingOperation_OnStatusChange;
            }
            if (_pendingOperations.Count > 0)
            {
                _currentPendingOperation = _pendingOperations[0];
                _currentPendingOperation.OnProgressChange += CurrentPendingOperation_OnProgressChange;
                _currentPendingOperation.OnStatusChange += CurrentPendingOperation_OnStatusChange;
            }
            else
            {
                _currentPendingOperation = null;
            }
        }

        private void CurrentPendingOperation_OnStatusChange(object sender, Events.StatusChangeEventArgs e)
        {
            _pendingOperationProxies[0].Status = e.Status;
             switch (e.Status)
             {
                 case PendingOperationStatus.Finished:
                     _pendingOperations.RemoveAt(0);
                     _pendingOperationProxies.RemoveAt(0);
                     bool reloadGame;

                     //insert any new pending operations as a result of the current operation finishing.
                     var newOperation = _currentPendingOperation.NewOperationOnFinish(_readerWriterLock,out reloadGame);
                     if (newOperation!=null)
                     {
                         _pendingOperations.Insert(0,newOperation);
                         _pendingOperationProxies.Add(new PendingOperation
                          {
                              Name = newOperation.Name,
                              Progress = 0,
                              Total = 1,
                              Status = PendingOperationStatus.Queued
                          });
                     }
                     SetCurrentOperation();

                     //refresh the game if necessary
                     if (reloadGame && OnReload!=null)
                     {
                         var args = new ReloadGameEventArgs();
                         OnReload(this,args);
                         if (args.Abort)
                         {
                             Error();
                         }
                     }

                    //save the current pending operations before starting the new operation
                    if (_currentPendingOperation != null)
                    {
                        Save();
                        _currentPendingOperation.Start();
                    }
                    else
                    {
                        ClearPendingOperationsQueue();
                        if (OnComplete != null)
                        {
                            OnComplete(this, new EventArgs());
                        }
                    }
                     break;
                 case PendingOperationStatus.Paused:
                     Save();
                     break;
                 case PendingOperationStatus.Error:
                     Error();
                     break;
             }
        }

        private void ClearPendingOperationsQueue()
        {
            //if the operations queue is empty, delete the file and notify that pending operations are now complete
            try
            {
                var queueFile = FileSystem.Current.GetFile(_pendingOperationQueueFile);
                if (queueFile.Exists) queueFile.Delete();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, "Unable to delete pending operations queue");
            }
        }

        private void Error()
        {
            //clear out all pending operations and notify that an error has occurred
            ClearPendingOperationsQueue();
            _pendingOperations.Clear();
            _pendingOperationProxies.Clear();
            if (OnError != null)
            {
                OnError(this, new EventArgs());
            }
        }

        private void CurrentPendingOperation_OnProgressChange(object sender, ProgressEventArgs e)
        {
            _pendingOperationProxies[0].Progress = e.Progress;
            _pendingOperationProxies[0].Total = e.Total;
        }

        //should only be called once a writelock is aquired.
        public void Start()
        {            
            if (_currentPendingOperation!=null)
            {            
                _currentPendingOperation.Start();
            } 
        }

        //should only be called once a writelock is aquired.
        public void Pause()
        {
            if (_currentPendingOperation != null)
            {
                _currentPendingOperation.Pause();
            } 
        }

        //should only be called once a writelock is aquired.
        //only callable via the uninstall operation to stop any other operations in progress.
        public void Cancel()
        {
            if (_currentPendingOperation != null)
            {
                _currentPendingOperation.Cancel();
            }
        }

        //should only be called once a readlock is aquired.
        public List<PendingOperation> GetPendingOperations()
        {
            var pendingOperations = new List<PendingOperation>();
            foreach (var pendingOperation in _pendingOperationProxies)
            {
                var pendingOperationCopy = new PendingOperation
                                               {
                                                   Name = pendingOperation.Name,
                                                   Progress = pendingOperation.Progress,
                                                   Total = pendingOperation.Total,
                                                   Status = pendingOperation.Status
                                               };
                pendingOperations.Add(pendingOperationCopy);
            }
            return pendingOperations;
        }
    }
}
