using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;

namespace MGDF.GamesManager.Model.Entities.XmlEntities
{
    class PendingOperationQueue: XmlEntity
    {
        private readonly List<IPendingOperation> _pendingOperations = new List<IPendingOperation>();
        private readonly ReaderWriterLockSlim _readerWriterLock;

        public List<IPendingOperation> PendingOperations
        {
            get 
            {
                return _pendingOperations;
            }
        }

        public PendingOperationQueue(string filename,ReaderWriterLockSlim readerWriterLock)
        {
            _readerWriterLock = readerWriterLock;
            Load(filename, null);
        }

        public PendingOperationQueue(List<IPendingOperation> pendingOperations)
        {
            _pendingOperations = pendingOperations;
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                // parse based on NodeType
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        switch (reader.Name)
                        {
                            case "pendinggamedownload":
                                _pendingOperations.Add(new PendingGameDownload(reader, _readerWriterLock));
                                break;
                            case "pendinggameinstall":
                                _pendingOperations.Add(new PendingGameInstall(reader, _readerWriterLock));
                                break;
                            case "pendinggameuninstall":
                                _pendingOperations.Add(new PendingGameUninstall(reader, _readerWriterLock));
                                break;
                        }
                        break;
                }
            }
        }

        public void Save(string filename)
        {
            XmlWriterSettings settings = new XmlWriterSettings
            {
                OmitXmlDeclaration = false,
                Indent = true,
                NewLineChars = "\r\n",
                IndentChars = "\t"
            };

            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Create))
            {
                using (var writer = XmlWriter.Create(stream, settings))
                {
                    writer.WriteStartElement("pendingoperationqueue");
                    foreach (var operation in _pendingOperations)
                    {
                        operation.Save(writer);
                    }
                    writer.WriteEndElement();
                }
            }
        }
    }
}
