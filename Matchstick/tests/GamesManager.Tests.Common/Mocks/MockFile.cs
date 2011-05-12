using System;
using System.IO;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
    public class MockFile : IFile
    {
        private readonly string _name;
        private readonly MockDirectory _parent;
        private MockStream _stream;

        public Version AssemblyVersion { get; set; }
        public Version FileVersion{ get; set; }

        public long Length
        {
            get
            {
                return _stream.Length;
            }
        }

        public MockFile(MockDirectory parent, string name)
        {
            _parent = parent;
            _name = name;

            _stream = null;
            AssemblyVersion = null;
            FileVersion = null;
        }

        public MockFile(MockDirectory parent, string name, string data)
        {
            _parent = parent;
            _name = name;

            WriteText(data);
        }

        public MockFile(MockDirectory parent, string name, byte[] data)
        {
            _parent = parent;
            _name = name;

            WriteBinary(data);
        }

        public string Name
        {
            get { return _name; }
        }

        public string FullName
        {
            get
            {
                var sb = new StringBuilder();
                IDirectory directory = _parent;
                while (directory != null)
                {
                    sb.Insert(0, directory.Name + @"\");
                    directory = directory.Parent;
                }

                sb.Append(_name);
                return sb.ToString();
            }
        }

        public void Delete()
        {
            AssertExists();
            _stream = null;
        }

        public bool Exists
        {
            get { return _stream != null; }
        }

        public void CopyTo(string path)
        {
            AssertExists();

            byte[] data = ReadBinary();

            MockFile file = (MockFile)FileSystem.Current.GetFile(path);
            file.WriteBinary(data);
            file.AssemblyVersion = AssemblyVersion;
            file.FileVersion = FileVersion;
        }

        public void MoveTo(string path)
        {
            AssertExists();

            byte[] data = ReadBinary();

            MockFile file = (MockFile)FileSystem.Current.GetFile(path);
            file.WriteBinary(data);
            file.AssemblyVersion = AssemblyVersion;
            file.FileVersion = FileVersion;

            Delete();
        }

        public string ReadText()
        {
            byte[] buffer = ReadBinary();
            return Encoding.UTF8.GetString(buffer);
        }

        public byte[] ReadBinary()
        {
            AssertExists();

            LastAccessTimeUtc = TimeService.Current.Now;

            _stream.Open();
            _stream.Seek(0, SeekOrigin.Begin);
            byte[] buffer = new byte[_stream.Length];
            _stream.Read(buffer, 0, buffer.Length);
            return buffer;
        }

        public void WriteText(string data)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(data);
            WriteBinary(bytes);
        }

        public void WriteBinary(byte[] data)
        {
            if (_parent == null) throw new DirectoryNotFoundException("Parent directory is null.");

            LastAccessTimeUtc = TimeService.Current.Now;
            LastWriteTimeUtc = TimeService.Current.Now;

            _stream = new MockStream();
            _stream.Open();
            _stream.Write(data, 0, data.Length);

            if (_parent.GetFiles(Name).Count == 0)
            {
                _parent.AddFile(this);
            }
        }

        public IDirectory Parent
        {
            get { return _parent; }
        }

        public DateTime LastAccessTimeUtc { get; set; }
        public DateTime LastWriteTimeUtc { get; set; }

        private void AssertExists()
        {
            if (_stream == null) throw new FileNotFoundException("File not found in test repository", FullName);
        }

        private void CreateFileStream()
        {
            if (_stream == null)
            {
                if (_parent.GetFiles(Name).Count == 0)
                {
                    _parent.AddFile(this);
                }
            }

            _stream = new MockStream();
        }

        public Stream OpenStream(FileMode mode)
        {
            switch (mode)
            {
                case FileMode.Create:
                case FileMode.Truncate:
                    CreateFileStream();
                    _stream.Open();
                    break;

                case FileMode.CreateNew:
                    if (_stream != null)
                    {
                        throw new IOException("File already exists in fake filesystem");
                    }
                    CreateFileStream();
                    _stream.Open();
                    break;

                case FileMode.Append:
                    if (_stream == null)
                    {
                        CreateFileStream();
                    }
                    _stream.Open();
                    _stream.Seek(0, SeekOrigin.End);
                    break;

                case FileMode.OpenOrCreate:
                    if (_stream == null)
                    {
                        CreateFileStream();
                    }
                    _stream.Open();
                    _stream.Seek(0, SeekOrigin.Begin);
                    break;

                case FileMode.Open:
                    if (_stream != null)
                    {
                        _stream.Open();
                        _stream.Seek(0, SeekOrigin.Begin);
                    }
                    break;
            }

            if (_stream != null)
            {
                return _stream;
            }

            throw new FileNotFoundException("File not found in fake filesystem", FullName);
        }

        public Stream OpenStream(FileMode mode, FileAccess access, FileShare share)
        {
            return OpenStream(mode);
        }
    }
}