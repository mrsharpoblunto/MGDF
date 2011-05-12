using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace MGDF.GamesManager.Model.Contracts.Messages
{
    [DataContract]
    public class ResponseBase
    {
        private List<string> _errors = new List<string>();

        [DataMember]
        public List<string> Errors
        {
            get { return _errors; }
            set { _errors = value; }
        }
    }
}
