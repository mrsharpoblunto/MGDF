using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Events;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    abstract class PendingOperationBase : XmlFragmentEntity, IPendingOperation
    {

        protected PendingOperationBase(XmlReader reader,ReaderWriterLockSlim readerWriterLock)
            : base(reader)
        {
            _readerWriterLock = readerWriterLock;
        }

        protected PendingOperationBase(ReaderWriterLockSlim readerWriterLock)
        {
            _readerWriterLock = readerWriterLock;
        }

        private readonly ReaderWriterLockSlim _readerWriterLock;
        private PendingOperationStatus _status = PendingOperationStatus.Queued;
        private uint _progress = 0;
        private uint _total = 1;

        protected bool PausePending { get; private set; }
        protected bool ErrorPending { get; private set; }

        public event EventHandler<StatusChangeEventArgs> OnStatusChange;
        public event EventHandler<ProgressEventArgs> OnProgressChange;
        public string Name { get; protected set; }

        public long Progress { get { return _progress; } }
        public long Total { get { return _total; } }
        public PendingOperationStatus Status { get { return _status; } }

        public void Start()
        {
            if (_status == PendingOperationStatus.Queued || _status == PendingOperationStatus.Paused)
            {
                if (Status == PendingOperationStatus.Paused)
                {
                    Logger.Current.Write(LogInfoLevel.Info,"Resuming operation: "+Name);
                }
                else
                {
                    Logger.Current.Write(LogInfoLevel.Info, "Starting operation: " + Name);
                }

                SetState(PendingOperationStatus.Working);
                PausePending = false;
                Thread thread = new Thread(DoWork);
                thread.Start();
            }
        }

        public void Pause()
        {
            if (_status == PendingOperationStatus.Working)
            {
                Logger.Current.Write(LogInfoLevel.Info, "Pausing operation: " + Name);

                SetState(PendingOperationStatus.Pausing);
                PausePending = true;
            }
        }

        public void Cancel()
        {
            if (_status == PendingOperationStatus.Working || _status == PendingOperationStatus.Paused)
            {
                Logger.Current.Write(LogInfoLevel.Info, "Cancelling operation: " + Name);
                ErrorPending = true;

                if (_status == PendingOperationStatus.Paused)
                {
                    SetState(PendingOperationStatus.Working);
                    Thread thread = new Thread(DoWork);
                    thread.Start();
                }
            }
        }

        private void DoWork(object param)
        {
            bool error = false;
            bool finished = false;
            bool paused = false;

            try
            {
                if (!ErrorPending && !PausePending) Work();
                if (ErrorPending)
                {
                    Logger.Current.Write(LogInfoLevel.Info, "Completed operation: " + Name+" with errors");
                    Error();
                    error = true;
                }
                else if (!PausePending)
                {
                    Logger.Current.Write(LogInfoLevel.Info, "Completed operation: " + Name);
                    Finished();
                    finished = true;
                }
                else
                {
                    Logger.Current.Write(LogInfoLevel.Info, "Paused operation: " + Name);
                    Paused();
                    paused = true;
                }
            }
            catch (Exception ex)
            {
                if (!ErrorPending)
                {
                    error = true;
                    Logger.Current.Write(ex, string.Format("Unexpected error while " + Name));
                }
            }

            _readerWriterLock.EnterWriteLock();
            try 
            {
                if (error)
                {
                    SetState(PendingOperationStatus.Error);
                }
                else if (finished)
                {
                    SetState(PendingOperationStatus.Finished);
                }
                else if (paused)
                {
                    SetState(PendingOperationStatus.Paused);
                }
            }
            finally
            {
                _readerWriterLock.ExitWriteLock();
            }
        }

        protected bool LoadBase(XmlReader reader)
        {
            switch (reader.NodeType)
            {
                case XmlNodeType.Element:
                    switch (reader.Name)
                    {
                        case "progress":
                            _progress = uint.Parse(reader.ReadString());
                            return true;

                        case "status":
                            _status = (PendingOperationStatus)Enum.Parse(typeof (PendingOperationStatus), reader.ReadString());
                            return true;

                        case "total":
                            _total = uint.Parse(reader.ReadString());
                            return true;
                    }
                    break;
            }
            return false;
        }

        protected void SaveBase(XmlWriter writer)
        {
            writer.WriteElementString("status", _status.ToString());
            writer.WriteElementString("progress", _progress.ToString());
            writer.WriteElementString("total", _total.ToString());
        }

        private void SetState(PendingOperationStatus status)
        {
            Logger.Current.Write(LogInfoLevel.Info, "Setting state for operation: " + Name+" - "+status);
            _status = status;
            if (OnStatusChange != null)
            {
                OnStatusChange(this, new StatusChangeEventArgs { Status = _status });
            }
        }

        protected void UpdateTotal(uint total)
        {
            _total = total;
            if (OnProgressChange!=null)
            {
                OnProgressChange(this,new ProgressEventArgs{Progress = 0,Total = _total});
            }
        }

        protected void UpdateTotal(long total)
        {
            uint uTotal = total > uint.MaxValue ? uint.MaxValue : (uint) total;
            UpdateTotal(uTotal);
        }

        protected void UpdateProgress(uint progress)
        {
            _progress = progress;
            if (OnProgressChange != null)
            {
                OnProgressChange(this, new ProgressEventArgs { Progress = _progress, Total = _total });
            } 
        }

        protected void UpdateProgress(long progress)
        {
            uint uProgress = progress > uint.MaxValue ? uint.MaxValue : (uint)progress;
            UpdateProgress(uProgress);
        }

        protected void Abort(string message)
        {
            ErrorPending = true;
            Logger.Current.Write(LogInfoLevel.Error,message);
        }

        public abstract IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock, out bool reloadGame);
        public abstract void Save(XmlWriter writer);

        protected abstract void Error();
        protected abstract void Paused();
        protected abstract void Finished();
        protected abstract void Work();

    }
}