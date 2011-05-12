using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model.Events
{
    class CopyProgressEventArgs : EventArgs
    {
        public long BytesCopied { get; set; }
        public long TotalBytesCopied { get; set; }
        public string CurrentFile { get; set; }
        public long CurrentFileSize { get; set; }
        public bool Cancel { get; set; }
    } 
}
