using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Common.Framework
{
    public interface IFile
    {
        string Name { get; }
        string FullName { get; }
        void Delete();
        bool Exists { get; }
        void CopyTo(string path);
        string ReadText();
        void WriteText(string data);
        IDirectory Parent { get; }
        DateTime LastAccessTimeUtc { get; set; }
        DateTime LastWriteTimeUtc { get; set; }
        Stream OpenStream(FileMode mode);
        Stream OpenStream(FileMode mode, FileAccess access, FileShare share);
        void WriteBinary(byte[] data);
        byte[] ReadBinary();
        Version AssemblyVersion { get; }
        Version FileVersion { get; }
        long Length { get; }
        void MoveTo(string path);
    }

    public class File : IFile
    {
        private FileInfo _fileInfo;

        public File(string path)
        {
            _fileInfo = new FileInfo(path);
        }

        public long Length
        {
            get { return _fileInfo.Length; }
        }

        public void MoveTo(string path)
        {
            _fileInfo.MoveTo(path);
        }

        public string Name
        {
            get { return _fileInfo.Name; }
        }

        public string FullName
        {
            get { return _fileInfo.FullName; }
        }

        public DateTime LastAccessTimeUtc
        {
            get { return _fileInfo.LastAccessTimeUtc; }
            set
            {
                if (FileSystem.Current.UpdateFileLastAccess) _fileInfo.LastAccessTimeUtc = value;
            }
        }

        public DateTime LastWriteTimeUtc
        {
            get { return _fileInfo.LastWriteTimeUtc; }
            set
            {
                if (FileSystem.Current.UpdateFileLastAccess) _fileInfo.LastWriteTimeUtc = value;
            }
        }

        public void Delete()
        {
            _fileInfo.Delete();
        }

        public bool Exists
        {
            get
            {
                _fileInfo = new FileInfo(FullName);
                return _fileInfo.Exists;
            }
        }

        public void CopyTo(string path)
        {
            _fileInfo.CopyTo(path, true);
        }

        public string ReadText()
        {
            try
            {
                LastAccessTimeUtc = TimeService.Current.Now;
            }
            catch
            {
                // if we don't have modify rights to the file an exception will occur
            }

            using (var reader = new StreamReader(FullName))
            {
                return reader.ReadToEnd();
            }
        }

        public void WriteText(string data)
        {
            using (var writer = new StreamWriter(FullName, false))
            {
                writer.Write(data);
            }
            LastAccessTimeUtc = TimeService.Current.Now;
        }

        public IDirectory Parent
        {
            get { return FileSystem.Current.GetDirectory(_fileInfo.DirectoryName); }
        }

        public Stream OpenStream(FileMode mode)
        {
            return new FileStream(_fileInfo.FullName, mode);
        }

        public Stream OpenStream(FileMode mode, FileAccess access, FileShare share)
        {
            return new FileStream(_fileInfo.FullName, mode, access, share);
        }

        public byte[] ReadBinary()
        {
            try
            {
                LastAccessTimeUtc = TimeService.Current.Now;
            }
            catch
            {
                // if we don't have modify rights to the file an exception will occur
            }

            using (FileStream inputStream = new FileStream(_fileInfo.FullName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
            {
                inputStream.Seek(0, SeekOrigin.Begin);
                byte[] buffer = new byte[inputStream.Length];
                inputStream.Read(buffer, 0, buffer.Length);
                return buffer;
            }
        }

        public Version AssemblyVersion
        {
            get
            {
                if (!Exists) return null;

                byte[] data = ReadBinary();
                Assembly assembly = Assembly.Load(data);
                return assembly.GetName().Version;
            }
        }

        public Version FileVersion
        {
            get
            {
                if (!Exists) return null;

                FileVersionInfo info = FileVersionInfo.GetVersionInfo(FullName);
                return new Version(Regex.Replace(info.FileVersion, @"\(.*\)", string.Empty).Trim());
            }
        }

        public void WriteBinary(byte[] data)
        {
            LastAccessTimeUtc = TimeService.Current.Now;

            using (FileStream outputStream = new FileStream(_fileInfo.FullName, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                outputStream.Write(data, 0, data.Length);
            }
        }
    }
}