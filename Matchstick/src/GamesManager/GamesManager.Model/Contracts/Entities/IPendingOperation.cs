using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Model.Events;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    [DataContract]
    public enum PendingOperationStatus
    {
        [EnumMember]
        Queued,
        [EnumMember]
        Working,
        [EnumMember]
        Paused,
        [EnumMember]
        Finished,
        [EnumMember]
        Error,
        [EnumMember]
        Pausing,
    }

    public interface IPendingOperation
    {
        event EventHandler<StatusChangeEventArgs> OnStatusChange;
        event EventHandler<ProgressEventArgs> OnProgressChange;

        long Progress { get; }
        long Total { get; }
        PendingOperationStatus Status { get; }
        string Name { get; }

        void Pause();
        void Start();
        void Cancel();

        IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock, out bool reloadGame);
        void Save(XmlWriter writer);
    }
}