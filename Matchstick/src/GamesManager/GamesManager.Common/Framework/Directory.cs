using System.Collections.Generic;
using System.IO;
using File=MGDF.GamesManager.Common.Framework.File;

namespace MGDF.GamesManager.Common.Framework
{
    public interface IDirectory
    {
        string Name { get; }
        string FullName { get; }
        List<IFile> GetFiles(string fileSpec);
        List<IDirectory> GetSubDirectories(string fileSpec);
        void Create();
        void Delete();
        bool Exists { get; }
        IDirectory Parent { get; }
        void CopyTo(string path);
        void Empty(params string[] exclusions);
	}

    public class Directory : IDirectory
    {
        private readonly DirectoryInfo _directoryInfo;

        public Directory(string path)
        {
            _directoryInfo = new DirectoryInfo(path);
        }

        public string Name
        {
            get { return _directoryInfo.Name; }
        }

        public string FullName
        {
            get { return _directoryInfo.FullName; }
        }

        public List<IFile> GetFiles(string fileSpec)
        {
            var files = new List<IFile>();

            foreach (FileInfo fileInfo in _directoryInfo.GetFiles(fileSpec))
            {
                IFile file = new File(fileInfo.FullName);
                files.Add(file);
            }

            return files;
        }

        public List<IDirectory> GetSubDirectories(string fileSpec)
        {
            var directories = new List<IDirectory>();

            foreach (DirectoryInfo directoryInfo in _directoryInfo.GetDirectories(fileSpec))
            {
                IDirectory directory = new Directory(directoryInfo.FullName);
                directories.Add(directory);
            }

            return directories;
        }

        public void Create()
        {
            _directoryInfo.Create();
        }

        public void Delete()
        {
            _directoryInfo.Delete(true);
        }

        public bool Exists
        {
            get { return _directoryInfo.Exists; }
        }

        public IDirectory Parent
        {
            get { 
                if ( _directoryInfo.Parent == null ) return null;
                return FileSystem.Current.GetDirectory(_directoryInfo.Parent.FullName); }
        }

        public void CopyTo(string path)
        {
            var _destination = new DirectoryInfo(path);
            RecursiveCopy(_directoryInfo, _destination, "*.*");
        }

        public void Empty(params string[] exclusions)
        {
            var exclusionsList = new List<string>(exclusions);

            // delete sub directories that are not in the exclusions list
            foreach (DirectoryInfo subDirectory in _directoryInfo.GetDirectories())
            {
                if (!exclusionsList.Contains(subDirectory.Name))
                {
                    subDirectory.Delete(true);
                }
            }

            // delete files that are not in the exclusions list
            foreach (FileInfo file in _directoryInfo.GetFiles())
            {
                if (!exclusionsList.Contains(file.Name))
                {
                    file.Delete();
                }
            }
        }

        private static void RecursiveCopy(DirectoryInfo source, DirectoryInfo destination, string fileSpec)
        {
            // create destination path if not already existing
            if (!destination.Exists) destination.Create();

            // copy all files that match the filespec
            FileInfo[] files = source.GetFiles(fileSpec);
            foreach (FileInfo file in files)
            {
                string target = Path.Combine(destination.FullName, file.Name);
                file.CopyTo(target, true);
            }

            // copy all files and subdirectories from source to destination
            foreach (DirectoryInfo subDirectory in source.GetDirectories())
            {
                var subDestination = new DirectoryInfo(Path.Combine(destination.FullName, subDirectory.Name));
                RecursiveCopy(subDirectory, subDestination, fileSpec);
            }
        }
    }
}