using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Model.Factories;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    class PendingGameDownload : PendingDownloadBase
    {
        private string _md5Hash;
        private string _gameUid;
        private IGameInstall _install;

        public PendingGameDownload(XmlReader reader,ReaderWriterLockSlim readerWriterLock)
            : base(reader, readerWriterLock)
        {
            Name = "Downloading Update";
        }

        public PendingGameDownload(GameSource.Contracts.Entities.GameVersionUpdate gameVersion, ReaderWriterLockSlim readerWriterLock)
            : base(readerWriterLock)
        {
            _destinationFileName = Path.Combine(Constants.DownloadsDir, IdentityGenerator.Current.GenerateIdentity() + ".mza");
            _sourceUrl = gameVersion.DownloadURL;
            _md5Hash = gameVersion.Md5Hash;
            _gameUid = gameVersion.Game.Uid;
            Name = "Downloading Update";
        }

        public override IPendingOperation NewOperationOnFinish(ReaderWriterLockSlim readerWriterLock,out bool reloadGame)
        {
            reloadGame = false;
            return new PendingGameInstall(_install, true, readerWriterLock);
        }

        protected override void Work()
        {
            base.Work();

            if (!PausePending && !ErrorPending)
            {
                string calculatedMd5 = FileSystem.Current.GetFile(_destinationFileName).ComputeMD5();
                if (_md5Hash != calculatedMd5)
                {
                    Abort(string.Format("Downloaded file {0} has an incorrect md5 file hash", _destinationFileName));
                }
                else
                {
                    _install = EntityFactory.Current.CreateGameInstall(_destinationFileName);
                    if (_install.IsValid)
                    {
                        if (_install.Game.Uid != _gameUid)
                        {
                            Abort(string.Format("Downloaded file {0} has an incorrect game uid {1}, expected {2}", _destinationFileName, _install.Game.Uid, _gameUid));
                        }
                    }
                    else
                    {
                        foreach (var error in _install.ErrorCollection)
                        {
                            Logger.Current.Write(LogInfoLevel.Error, string.Format("Downloaded file {0} is not a valid installer - {1}", _destinationFileName, error));
                        }
                        Abort("Aborting download of invalid installer");
                    }
                }
            }
        }

        protected override void Load(XmlReader reader)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        if (reader.Name.Equals("pendinggamedownload"))
                        {
                            return;
                        }
                        break;

                    case XmlNodeType.Element:
                        if (!LoadBase(reader))
                        {
                            switch (reader.Name)
                            {
                                case "destinationfilename":
                                    _destinationFileName = reader.ReadString();
                                    break;

                                case "sourceurl":
                                    _sourceUrl = reader.ReadString();
                                    break;

                                case "md5hash":
                                    _md5Hash = reader.ReadString();
                                    break;

                                case "gameuid":
                                    _gameUid = reader.ReadString();
                                    break;
                            }
                        }
                        break;
                }
            }
        }

        public override void Save(XmlWriter writer)
        {
            writer.WriteStartElement("pendinggamedownload");
            SaveBase(writer);
            writer.WriteElementString("destinationfilename", _destinationFileName);
            writer.WriteElementString("sourceurl", _sourceUrl);
            writer.WriteElementString("md5hash", _md5Hash);
            writer.WriteElementString("gameuid", _gameUid);
            writer.WriteEndElement();
        }

        protected override void Finished()
        {
        }

        protected override void Error()
        {
            try
            {
                FileSystem.Current.GetFile(_destinationFileName).DeleteWithTimeout();
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex, string.Format("Unable to delete pending download file '" + _destinationFileName + "'"));
            }
        }

        protected override void Paused()
        {
        }
    }
}