using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using MGDF.GamesManager.Model.Contracts.Messages;

namespace MGDF.GamesManager.Model.Contracts.Entities
{
    [DataContract]
    public enum InstallState
    {
        [EnumMember]
        Error,
        [EnumMember]
        NotInstalled,
        [EnumMember]
        Updating,
        [EnumMember]
        Installed
    }

    [DataContract]
    public class InstalledGameInfo
    {
        public InstalledGameInfo()
        {
            PendingOperations = new List<PendingOperation>();
        }

        [DataMember]
        public List<PendingOperation> PendingOperations { get; private set; }

        [DataMember]
        public InstallState InstallState { get; set;}

        [DataMember]
        public string Version { get; set;}
    }
}
