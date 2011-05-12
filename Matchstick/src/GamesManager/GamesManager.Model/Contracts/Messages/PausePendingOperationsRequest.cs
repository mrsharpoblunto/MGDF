using System.Runtime.Serialization;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class PausePendingOperationsRequest : RequestBase
    {
        [DataMember]
        public string GameUid { get; set; }
    }
}