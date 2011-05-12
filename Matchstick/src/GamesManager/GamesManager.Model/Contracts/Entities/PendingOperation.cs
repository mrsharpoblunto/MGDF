using System.Runtime.Serialization;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    [DataContract]
    public class PendingOperation
    {
        [DataMember]
        public PendingOperationStatus Status { get; set; }
        [DataMember]
        public long Progress { get; set; }
        [DataMember]
        public long Total { get; set; }
        [DataMember]
        public string Name { get; set; }
    }
}