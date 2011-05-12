using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Model.Helpers;
using File=MGDF.GamesManager.Common.Framework.File;

namespace MGDF.GamesManager.Model.ServiceModel
{
    public class FrameworkUpdateManager: IDisposable
    {
        public event EventHandler OnStarted;
        public event EventHandler OnComplete;


        private static readonly object _instanceLock = new object();
        private static FrameworkUpdateManager _instance;

        private readonly ReaderWriterLockSlim _readerWriterLock;
        private PendingFrameworkDownload _pendingFrameworkDownload;


        private Thread _updateThread;

        public static FrameworkUpdateManager Instance
        {
            get
            {
                if (_instance == null)
                {
                    lock (_instanceLock)
                    {
                        if (_instance == null)
                        {
                            _instance = new FrameworkUpdateManager();
                        }
                    }
                }
                return _instance;
            }
        }

        private FrameworkUpdateManager()
        {
            _readerWriterLock = new ReaderWriterLockSlim();
        }

        public void Start()
        {
            UpdateFrameworkUpdater();
            if (_updateThread == null)
            {
                _updateThread = new Thread(DoUpdates);
                _updateThread.Start();
            }
        }

        private void DoUpdates()
        {
            IFile pendingFrameworkDownload = FileSystem.Current.GetFile(Constants.FrameworkUpdateFile);
            if (pendingFrameworkDownload.Exists)
            {
                using (var stream = pendingFrameworkDownload.OpenStream(FileMode.Open))
                {
                    using (var reader = XmlReader.Create(stream))
                    {
                        _pendingFrameworkDownload = new PendingFrameworkDownload(reader, _readerWriterLock);
                    }
                }
            }

            var availableVersion = FrameworkUpdateChecker.CheckLatestVersion();

            //there is a currently available version newer than the version being downloaded. cancel the current download
            //and start downloading the new version.
            if (availableVersion!=null && _pendingFrameworkDownload != null && availableVersion.Version > _pendingFrameworkDownload.Version)
            {
                Logger.Current.Write(LogInfoLevel.Info, "Found new Framework version " + availableVersion.Version + ", cancelling download of version " + _pendingFrameworkDownload.Version+"...");
                FileSystem.Current.GetFile(_pendingFrameworkDownload.DestinationFileName).DeleteWithTimeout();
                _pendingFrameworkDownload = new PendingFrameworkDownload(availableVersion, _readerWriterLock);
                Save();
            }
            else if (availableVersion!=null && _pendingFrameworkDownload==null)
            {
                //if the available update hasn't already been downloaded and is greater than the currently installed framework version, then begin downloading the new one.
                IFile existingDownload = FileSystem.Current.GetFile(Path.Combine(Constants.FrameworkUpdatesDir, availableVersion.Version + ".exe"));
                IFile gamesManager = FileSystem.Current.GetFile(Constants.GamesManagerExecutable);

                if (!existingDownload.Exists && gamesManager.Exists && availableVersion.Version > gamesManager.AssemblyVersion)
                {
                    Logger.Current.Write(LogInfoLevel.Info, "Found new Framework version " + availableVersion.Version + ", Downloading...");
                    _pendingFrameworkDownload = new PendingFrameworkDownload(availableVersion, _readerWriterLock);
                    Save();
                }
            }

            if (_pendingFrameworkDownload!=null)
            {
                _pendingFrameworkDownload.OnStatusChange += PendingFrameworkDownload_OnStatusChange;
                _pendingFrameworkDownload.Start();
                if (OnStarted!=null)
                {
                    OnStarted(this, new EventArgs());
                }
            }
        }

        void PendingFrameworkDownload_OnStatusChange(object sender, Events.StatusChangeEventArgs e)
        {
            switch (e.Status)
            {
                case PendingOperationStatus.Finished:
                    _pendingFrameworkDownload.OnStatusChange-= PendingFrameworkDownload_OnStatusChange;
                    FileSystem.Current.GetFile(Constants.FrameworkUpdateFile).DeleteWithTimeout();
                    _updateThread = null;
                    _pendingFrameworkDownload = null;
                    if (OnComplete!=null)
                    {
                        OnComplete(this,new EventArgs());
                    }
                    break;
                case PendingOperationStatus.Paused:
                    _pendingFrameworkDownload.OnStatusChange -= PendingFrameworkDownload_OnStatusChange;
                    Save();
                    _updateThread = null;
                    _pendingFrameworkDownload = null;
                    break;
                case PendingOperationStatus.Error:
                    _pendingFrameworkDownload.OnStatusChange -= PendingFrameworkDownload_OnStatusChange;
                    FileSystem.Current.GetFile(_pendingFrameworkDownload.DestinationFileName).DeleteWithTimeout();
                    FileSystem.Current.GetFile(Constants.FrameworkUpdateFile).DeleteWithTimeout();
                    _updateThread = null;
                    _pendingFrameworkDownload = null;
                    if (OnComplete != null)
                    {
                        OnComplete(this, new EventArgs());
                    }
                    break;
            }
        }

        private void Save()
        {
            XmlWriterSettings settings = new XmlWriterSettings
                                             {
                                                 OmitXmlDeclaration = false,
                                                 Indent = true,
                                                 NewLineChars = "\r\n",
                                                 IndentChars = "\t"
                                             };
            IFile pendingFrameworkDownload = FileSystem.Current.GetFile(Constants.FrameworkUpdateFile);
            using (var stream = pendingFrameworkDownload.OpenStream(FileMode.Create))
            {
                using (var writer = XmlWriter.Create(stream, settings))
                {
                    _pendingFrameworkDownload.Save(writer);
                }
            }
        }

        public void Pause()
        {
            var pendingFrameworkDownload = _pendingFrameworkDownload;
            if (pendingFrameworkDownload != null)
            {
                pendingFrameworkDownload.Pause();
            }
        }

        public void Dispose()
        {
            try
            {
                Pause();
                while (_pendingFrameworkDownload != null)
                {
                    Thread.Sleep(10);
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Error Disposing FrameworkUpdateManager");    
            }

            _instance = null;
        }

        private void UpdateFrameworkUpdater()
        {
            //replace the old framework bootstrapper with a more recent one if available.
            if (FileSystem.Current.FileExists(Constants.FrameworkUpdaterExecutable))
            {
                try
                {
                    if (FileSystem.Current.FileExists(Constants.CurrentFrameworkUpdaterExecutable))
                    {
                        FileSystem.Current.GetFile(Constants.CurrentFrameworkUpdaterExecutable).DeleteWithTimeout();
                    }
                    if (FileSystem.Current.FileExists(Constants.CurrentFrameworkUpdaterConfig))
                    {
                        FileSystem.Current.GetFile(Constants.CurrentFrameworkUpdaterConfig).DeleteWithTimeout();
                    }
                    FileSystem.Current.GetFile(Constants.FrameworkUpdaterExecutable).MoveTo(Constants.CurrentFrameworkUpdaterExecutable);
                    FileSystem.Current.GetFile(Constants.FrameworkUpdaterConfig).MoveTo(Constants.CurrentFrameworkUpdaterConfig);

                }
                catch (Exception)
                {
                    //file is in use, we can't rename it now.    
                }
            }
        }
    }
}