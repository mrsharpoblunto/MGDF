using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.GameSource.Contracts.Entities
{
    [DataContract]
    public class GameVersion : GameVersionBase
    {
        [DataMember]
        public string DownloadURL { get; set; }

        [DataMember]
        public bool IsComplete { get; set; }

        public override string ToString()
        {
            return (IsUpdate ? ("Update " + UpdateMinVersion + " to " + Version) : Version) + (IsComplete ? string.Empty : " [INCOMPLETE]");
        }
    }
}
