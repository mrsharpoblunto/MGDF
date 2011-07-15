using System;
using System.Collections.Generic;
using System.IO;
using ICSharpCode.SharpZipLib.Zip;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model
{
    public interface IArchiveFactory
    {
        IArchiveFile OpenArchive(string filename);
    }

    public interface IArchiveFile: IDisposable
    {
        IArchiveFile Parent { get; }
        Dictionary<string, IArchiveFile> Children { get; }
        string Name { get;}
        bool IsFolder { get; }
        long Size { get; }

        Stream OpenStream();

        IArchiveFile GetFile(string filename);
    }

    internal class Archive : IArchiveFile
    {
        public IArchiveFile Parent { get; private set; }
        public Dictionary<string, IArchiveFile> Children { get; private set; }
        public string Name { get; private set; }
        public bool IsFolder { get; private set; }

        public long Size
        {
            get { return _entry.Size; }
        }

        private readonly ZipEntry _entry;
        private readonly ZipFile _file;

        public Archive(ZipFile file, IArchiveFile parent, string name, ZipEntry entry)
        {
            Parent = parent;
            Name = name;
            IsFolder = false;
            _entry = entry;
            _file = file;

            Children = new Dictionary<string, IArchiveFile>();
        }

        public Archive(ZipFile file, IArchiveFile parent, string name)
        {
            Parent = parent;
            Name = name;
            IsFolder = true;
            _file = file;

            Children = new Dictionary<string, IArchiveFile>();
        }

        public Stream OpenStream()
        {
            if (_entry!=null)
            {
                return _file.GetInputStream(_entry);
            }
            else
            {
                return null;
            }
        }

        public IArchiveFile GetFile(string filename)
        {
            IArchiveFile parent = this;
            string[] pathComponents = filename.ToLowerInvariant().Split(new []{'/','\\'},StringSplitOptions.RemoveEmptyEntries);

            foreach(var component in pathComponents)
            {
                if (parent.Children.ContainsKey(component))
                {
                    parent = parent.Children[component];
                }
                else
                {
                    return null;
                }
            }

            return parent;
        }

        public void Dispose()
        {
            if (Parent==null)
            {
                _file.Close();
            }
        }
    }

    public class ArchiveFactory : IArchiveFactory
    {
        public static IArchiveFactory Current
        {
            get
            {
                return ServiceLocator.Current.Get<IArchiveFactory>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public IArchiveFile OpenArchive(string filename)
        {
            var file = new ZipFile(filename);
            var root = new Archive(file, null, filename);
            foreach (ZipEntry entry in file)
            {
                string name = entry.Name.ToLowerInvariant();
                string[] pathComponents = name.Split(new[] { '/','\\' },StringSplitOptions.RemoveEmptyEntries);

                if (entry.IsFile)
                {
                    IArchiveFile parent = pathComponents.Length > 1 ? CreateDirectories(file, root, pathComponents, pathComponents.Length-1) : root;
                    IArchiveFile child = new Archive(file, parent, pathComponents[pathComponents.Length - 1], entry);
                    parent.Children.Add(child.Name, child);
                }
                else
                {
                    CreateDirectories(file, root, pathComponents, pathComponents.Length);                
                }
            }
            return root;
        }

        private static IArchiveFile CreateDirectories(ZipFile file, IArchiveFile root,string[] components,int length)
        {
            IArchiveFile parent = root;
            for (int i = 0; i < length; ++i)
            {
                if (!parent.Children.ContainsKey(components[i]))
                {
                    var child = new Archive(file,parent, components[i]);
                    parent.Children.Add(child.Name, child);
                    parent = child;
                }
                else
                {
                    parent = parent.Children[components[i]];
                }
            }
            return parent;
        }
    }
}