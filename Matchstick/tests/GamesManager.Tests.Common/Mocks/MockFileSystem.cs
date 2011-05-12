using System;
using System.Collections.Generic;
using System.IO;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockFileSystem : IFileSystem
    {
        private List<MockDirectory> _repository;

        public bool DirectoryExists(string directory)
        {
            return GetDirectory(directory).Exists;
        }

        public bool UpdateFileLastAccess { get; set; }

        public MockFileSystem()
        {
            _repository = new List<MockDirectory>();
            UpdateFileLastAccess = true;
        }

        public MockDirectory AddVolumeRoot(string volumeName)
        {
            MockDirectory volume = new MockDirectory(volumeName);
            _repository.Add(volume);
            return volume;
        }

        public IDirectory GetDirectory(string path)
        {
            path = path.Replace("/", "\\");
            string[] pathParts = path.Split(new char[]{'\\'}, StringSplitOptions.RemoveEmptyEntries);

            // determine if the root location is a local or remote path
            string volumeName = path.StartsWith(@"\\") ? "\\\\" + pathParts[0] + '\\' + pathParts[1] : pathParts[0];

            MockDirectory directory = _repository.Find(directoryToFind => directoryToFind.Name.ToLower() == volumeName.ToLower());
            if (directory == null)
            {
                throw new InvalidOperationException("The volume specified in the path was not found in the test repository: " + path);
            }

            // if this is the volume root, then just return now
            if (pathParts.Length == 1)
            {
                return directory;
            }

            int level = 1;

            bool found = true;
            while (found)
            {
                found = false;

                foreach (MockDirectory subDirectory in directory.GetSubDirectories("*"))
                {
                    if (subDirectory.Name.ToLower() == pathParts[level].ToLower())
                    {
                        level++;

                        if (level == pathParts.Length)
                        {
                            return subDirectory;
                        }
                        else
                        {
                            directory = subDirectory;
                            found = true;
                            break;
                        }
                    }
                }
            }

            return new MockDirectory(directory, path, false);
        }

        public IFile GetFile(string path)
        {
            try
            {
                path = path.Replace("/", "\\");
                string directoryPath = path.Substring(0, path.LastIndexOf(@"\"));
                string filename = path.Substring(directoryPath.Length + 1);

                MockDirectory directory = (MockDirectory)GetDirectory(directoryPath);
                if (directory.Exists)
                {
                    List<IFile> files = directory.GetFiles(filename);
                    if (files.Count != 0) return files[0];
                }

                // if the file is not found in the repository then return a stub for a non-existent file
                return new MockFile(directory, filename);
            }
            catch (Exception)
            {
                throw new InvalidOperationException("The format of the path supplied is invalid: " + path);
            }
        }
    
        public bool FileExists(string filename)
        {
            return GetFile(filename).Exists;
        }
    }
}