using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Contracts.Messages;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.Model.Factories;
using Game=MGDF.GamesManager.Model.Entities.XmlEntities.Game;

namespace MGDF.GamesManager.Model.ServiceModel
{
    class InstalledGame
    {
        private readonly ReaderWriterLockSlim _readerWriterLock = new ReaderWriterLockSlim();
        private PendingOperationRunner _pendingOperations;
        private InstallState _state;
        private IGame _game;
        private bool _pendingUninstall;

        public InstalledGameInfo GetInfo()
        {
            _readerWriterLock.EnterReadLock();
            try
            {
                var info = new InstalledGameInfo { InstallState = _state, Version = _game!=null ? _game.Version.ToString() : null};
                if (_pendingOperations != null) info.PendingOperations.AddRange(_pendingOperations.GetPendingOperations());
                return info;
            }
            finally
            {
                _readerWriterLock.ExitReadLock();
            }
        }

        public IGame Game
        {
            get { return _game;  }
        }

        public void Start()
        {
            var pending = _pendingOperations;

            _readerWriterLock.EnterUpgradeableReadLock();
            try
            {
                if (pending != null)
                {
                    _readerWriterLock.EnterWriteLock();
                    try
                    {
                        pending.Start();
                    }
                    finally
                    {
                        _readerWriterLock.ExitWriteLock();
                    }
                }
            }
            finally
            {
                _readerWriterLock.ExitUpgradeableReadLock();
            }
        }

        public void Pause()
        {
            var pending = _pendingOperations;

            _readerWriterLock.EnterUpgradeableReadLock();
            try
            {
                if (pending != null)
                {
                    _readerWriterLock.EnterWriteLock();
                    try
                    {
                        pending.Pause();
                    }
                    finally
                    {
                        _readerWriterLock.ExitWriteLock();
                    }
                }
            }
            finally
            {
                _readerWriterLock.ExitUpgradeableReadLock();
            }
        }

        public void Update(List<GameVersionUpdate> versions, List<string> errors)
        {
            _readerWriterLock.EnterUpgradeableReadLock();
            try
            {
                if (_state!=InstallState.Installed)
                {
                    errors.Add("Can't update as this game is not installed");
                    return;
                }

                //ensure each update is an update, and can update the currently installed version.
                versions.Sort((a, b) => a.Version.CompareTo(b.Version));
                Version currentVersion = _game.Version;
                foreach (var v in versions)
                {
                    if (!v.IsUpdate)
                    {
                        errors.Add("Game version is not an update");
                        return;
                    }
                    else if (currentVersion < new Version(v.UpdateMinVersion) || currentVersion > new Version(v.UpdateMaxVersion))
                    {
                        errors.Add("Game version cannot update the installed version");
                        return;
                    }
                    currentVersion = new Version(v.Version);
                }

                if (_pendingOperations==null)
                {
                    _readerWriterLock.EnterWriteLock();
                    try
                    {
                        var operations = new List<IPendingOperation>();
                        foreach (var version in versions)
                        {
                            operations.Add(new PendingGameDownload(version, _readerWriterLock));
                        }
                        StartPendingOperations( operations.ToArray());
                    }
                    finally
                    {
                        _readerWriterLock.ExitWriteLock();
                    }
                }
                else
                {
                    errors.Add("Pending operations already in progress");
                }
            }
            finally
            {
                _readerWriterLock.ExitUpgradeableReadLock();
            }
        }

        public void Install(IGameInstall installer, List<string> errors)
        {
            if (!installer.IsUpdate)
            {
                _readerWriterLock.EnterUpgradeableReadLock();
                try
                {
                    if (_state == InstallState.NotInstalled || _state == InstallState.Error)
                    {
                        if (_pendingOperations==null)
                        {
                            _readerWriterLock.EnterWriteLock();
                            try
                            {
                                IDirectory gameDir = FileSystem.Current.GetDirectory(Constants.GameDir(installer.Game.Uid));
                                if (!gameDir.Exists)
                                {
                                    gameDir.Create();
                                }
                                _game = installer.Game;
                                StartPendingOperations(new PendingGameInstall(installer, false, _readerWriterLock));
                            }
                            finally
                            {
                                _readerWriterLock.ExitWriteLock();
                            }        
                        }
                        else
                        {
                            errors.Add("Pending operations already in progress");
                        }
                    }
                    else
                    {
                        errors.Add("Game already installed");
                    }
                }
                finally
                {
                    _readerWriterLock.ExitUpgradeableReadLock();
                }
            }
            else
            {
                 _readerWriterLock.EnterUpgradeableReadLock();
                try
                {
                    if (_state == InstallState.Installed)
                    {
                        if (_game.Version < installer.Update.UpdateMinVersion || _game.Version > installer.Update.UpdateMaxVersion)
                        {
                            errors.Add("Game version cannot update the installed version");
                            return;
                        }
                        else if (_pendingOperations==null)
                        {
                            _readerWriterLock.EnterWriteLock();
                            try
                            {
                                StartPendingOperations(new PendingGameInstall(installer, false, _readerWriterLock));
                            }
                            finally
                            {
                                _readerWriterLock.ExitWriteLock();
                            }   
                        }
                        else
                        {
                            errors.Add("Pending operations already in progress");
                        }
                    }
                    else
                    {
                        errors.Add("Cannot install an update-only game version");
                    }
                }
                finally
                {
                    _readerWriterLock.ExitUpgradeableReadLock();
                }
            }
        }

        public void Uninstall(string gameUid, List<string> errors)
        {
            var pending = _pendingOperations;

            _readerWriterLock.EnterUpgradeableReadLock();
            try
            {
                if (_state == InstallState.Updating && pending.GetPendingOperations().Count > 0 && pending.GetPendingOperations()[0].Name=="Uninstalling")
                {
                    errors.Add("Game is already being uninstalled");
                }
                else if (_state!=InstallState.NotInstalled)
                {
                    _readerWriterLock.EnterWriteLock();
                    try
                    {
                        if (pending != null)
                        {
                            //cancel whateve we're donig and queue up an uninstall
                            pending.Cancel();
                            _pendingUninstall = true;
                        }
                        else
                        {
                            StartPendingOperations(new PendingGameUninstall(_game, _readerWriterLock));
                        }
                    }
                    finally
                    {
                        _readerWriterLock.ExitWriteLock();
                    }   
                }
                else
                {
                    errors.Add("Game is not installed");
                }
            }
            finally
            {
                _readerWriterLock.ExitUpgradeableReadLock();
            }
        }

        public InstalledGame()
        {
            _state = InstallState.NotInstalled;
        }

        public InstalledGame(IDirectory directory)
        {
            _game = Load(directory);
            _pendingOperations = LoadPendingOperations(directory);
        }

        private IGame Load(IDirectory directory)
        {
            if (!directory.Exists)
            {
                _state = InstallState.NotInstalled;
                return null;
            }

            try
            {
                if (FileSystem.Current.FileExists(FileSystem.Combine(directory.FullName, Constants.GameConfig)))
                {
                    IGame game = EntityFactory.Current.CreateGame(FileSystem.Combine(directory.FullName, Constants.GameConfig));
                    if (game.ErrorCollection.Count > 0)
                    {
                        Logger.Current.Write(LogInfoLevel.Error, game.ErrorCollection[0]);
                        _state = InstallState.Error;
                    }
                    else if (FileSystem.Current.FileExists(Path.Combine(Constants.GameDir(game.Uid),Constants.ErrorFile)))
                    {
                        Logger.Current.Write(LogInfoLevel.Error, "Game is installed with errors");
                        _state = InstallState.Error;
                    }
                    else if (game.InterfaceVersion == Constants.InterfaceVersion) //only load compatible games
                    {
                        _state = InstallState.Installed;
                        return game;
                    }
                    else
                    {
                        Logger.Current.Write(LogInfoLevel.Error, string.Format("Incompatible MGDF Interface Version. Expected {0}, was {1}", Constants.InterfaceVersion, game.InterfaceVersion));
                        _state = InstallState.Error;
                    }
                }
                else
                {
                    _state = InstallState.NotInstalled;
                }
            }
            catch (Exception e)
            {
                Logger.Current.Write(e, string.Format("Unexpected error loading file \"{0}\": {1}", FileSystem.Combine(directory.FullName, Constants.GameConfig)));
                _state = InstallState.Error;
            }
            return null;
        }

        private PendingOperationRunner LoadPendingOperations(IDirectory directory)
        {
            var pendingOperationQueueFile = FileSystem.Current.GetFile(FileSystem.Combine(directory.FullName, Constants.PendingOperationQueue));
            if (pendingOperationQueueFile.Exists)
            {
                var runner = new PendingOperationRunner(pendingOperationQueueFile.FullName, _readerWriterLock);
                if (runner.GetPendingOperations().Count>0)
                {
                    _state = InstallState.Updating;
                    runner.OnComplete += Runner_OnComplete;
                    runner.OnError += Runner_OnError;
                    runner.OnReload += Runner_OnReload;
                    return runner;
                }
                else
                {
                    pendingOperationQueueFile.Delete();
                }
            }
            return null;
        }

        private void Runner_OnReload(object sender, ReloadGameEventArgs e)
        {
            _game = Load(FileSystem.Current.GetDirectory(Constants.GameDir(_game.Uid)));
            //if the operation that invoked the reload event put the game into an error state then abort further pending operations
            if (_state == InstallState.Error) e.Abort = true; 
        }

        private void Runner_OnError(object sender, EventArgs e)
        {
            CleanupPendingOperations();
            _game = Load(FileSystem.Current.GetDirectory(Constants.GameDir(_game.Uid)));
            if (_pendingUninstall)
            {
                _pendingUninstall = false;
                StartPendingOperations(new PendingGameUninstall(_game, _readerWriterLock));
            }
        }

        private void Runner_OnComplete(object sender, EventArgs e)
        {
            _pendingUninstall = false;
            CleanupPendingOperations();
        }

        private void CleanupPendingOperations()
        {
            if (_pendingOperations!=null)
            {
                _pendingOperations.OnComplete -= Runner_OnComplete;
                _pendingOperations.OnError -= Runner_OnError;
                _pendingOperations.OnReload -= Runner_OnReload;
                _pendingOperations = null;
            }
        }

        private void StartPendingOperations(params IPendingOperation[] operations)
        {
            _pendingOperations = new PendingOperationRunner(_game.Uid, _readerWriterLock, operations);
            _state = InstallState.Updating;
            _pendingOperations.OnComplete += Runner_OnComplete;
            _pendingOperations.OnError += Runner_OnError;
            _pendingOperations.OnReload += Runner_OnReload;
            _pendingOperations.Start();
        }
    }
}
