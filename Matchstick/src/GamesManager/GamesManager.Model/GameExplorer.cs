using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model
{
    public enum TaskCount : uint
    {
        One = 0,
        Two = 1,
        Three = 2,
        Four = 3,
        Five = 4
    }

    public interface IGameExplorer
    {
        bool IsInstalled(string gdfBinPath);
        void AddGameTask(Guid guid, TaskCount count, string name, string path, string arguments);
        void UninstallGame(string gdfBinPath);
        Guid InstallGame(string gdfBinPath, string gameInstallPath, string gameExecutable,string arguments);
    }

    enum GameInstallScope : uint
    {
        CurrentUser = 2,
        AllUsers = 3,
    }

    enum TaskType : uint
    {
        PlayTask = 0,
        SupportTask = 1,
    }

    /// <summary>
    /// Add/remove Games to the Vista games explorer, if not on vista, the install/uninstall actions do nothing
    /// </summary>
    public class GameExplorer: IGameExplorer 
    {
        public static IGameExplorer Current
        {
            get
            {
                return ServiceLocator.Current.Get<IGameExplorer>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        private const int Success = 0;

        [DllImport("gameuxinstallhelper.dll")]
        static extern int AddToGameExplorer(string gdfBinPath, string gameInstallPath,
                                            GameInstallScope scope, [In] ref Guid instanceGuid);

        [DllImport("gameuxinstallhelper.dll")]
        static extern int RemoveFromGameExplorer([In] ref Guid instanceGuid);

        [DllImport("gameuxinstallhelper.dll")]
        static extern int RetrieveGUIDForApplication(string gdfBinPath, out Guid instanceGuid);

        [DllImport("gameuxinstallhelper.dll")]
        static extern int RemoveTasks([In] ref Guid instanceGuid);

        [DllImport("gameuxinstallhelper.dll")]
        static extern int CreateTask(GameInstallScope scope, [In] ref Guid instanceGuid, TaskType taskType, uint count, string name, string path, string arguments);

        public bool IsInstalled(string gdfBinPath)
        {
            if (EnvironmentSettings.Current.OSVersion.Major >= 6)
            {
                Guid guid;
                if (Success != RetrieveGUIDForApplication(gdfBinPath, out guid))
                    return false;
            }
            return true;
        }

        public void AddGameTask(Guid guid, TaskCount count, string name, string path, string arguments)
        {
            if (EnvironmentSettings.Current.OSVersion.Major >= 6)
            {
                if (Success !=
                    CreateTask(GameInstallScope.AllUsers, ref guid, TaskType.SupportTask, (uint)count, name, path, arguments))
                {
                    throw new Exception("unable to add task to task explorer.");
                }
            }
        }

        public void UninstallGame(string gdfBinPath)
        {
            if (EnvironmentSettings.Current.OSVersion.Major >= 6)
            {
                Guid guid;
                if (Success != RetrieveGUIDForApplication(gdfBinPath, out guid))
                    throw new Exception("failed retrieving guid; game is probably not installed.");

                if (Success != RemoveTasks(ref guid))
                    throw new Exception("failed removing game: access denied");

                if (Success != RemoveFromGameExplorer(ref guid))
                    throw new Exception("failed removing game: access denied");
            }
        }

        public Guid InstallGame(string gdfBinPath, string gameInstallPath,string gameExecutable,string arguments)
        {
            if (EnvironmentSettings.Current.OSVersion.Major >= 6)
            {
                Guid guid = Guid.NewGuid();
                if (Success != AddToGameExplorer(gdfBinPath, gameInstallPath, GameInstallScope.AllUsers, ref guid))
                    throw new Exception("failed installing game: access denied.");

                if (Success !=
                    CreateTask(GameInstallScope.AllUsers, ref guid, TaskType.PlayTask, 0, "Play", gameExecutable, arguments))
                {
                    throw new Exception("failed adding play task.");
                }
                return guid;
            }
            return Guid.Empty;
        }

    }
}