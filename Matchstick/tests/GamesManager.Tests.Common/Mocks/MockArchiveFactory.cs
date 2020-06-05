using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockArchiveFactory : IArchiveFactory
  {
    public MockArchiveFactory()
    {
      VirtualArchives = new Dictionary<string, MockArchiveFile>();
    }

    public Dictionary<string, MockArchiveFile> VirtualArchives { get; private set; }

    public IArchiveFile OpenArchive(string filename)
    {
      return VirtualArchives[filename];
    }
  }

  public class MockArchiveFile : IArchiveFile
  {
    private readonly string _data;

    public MockArchiveFile(IArchiveFile parent, string name, string data)
    {
      Parent = parent;
      Name = name;
      IsFolder = false;
      _data = data;
      Children = new Dictionary<string, IArchiveFile>();
      Size = data.Length;

      if (parent != null)
      {
        parent.Children.Add(name.ToLowerInvariant(), this);
      }
    }

    public MockArchiveFile(IArchiveFile parent, string name)
    {
      Parent = parent;
      Name = name;
      IsFolder = true;
      Children = new Dictionary<string, IArchiveFile>();

      if (parent != null)
      {
        parent.Children.Add(name.ToLowerInvariant(), this);
      }
    }

    public void Dispose()
    {
    }

    public IArchiveFile Parent { get; private set; }
    public Dictionary<string, IArchiveFile> Children { get; private set; }
    public string Name { get; private set; }
    public bool IsFolder { get; private set; }
    public long Size { get; private set; }

    public Stream OpenStream()
    {
      return new MemoryStream(Encoding.UTF8.GetBytes(_data));
    }

    public IArchiveFile GetFile(string filename)
    {
      IArchiveFile parent = this;
      string[] pathComponents = filename.ToLowerInvariant().Split(new[] { '/' });

      foreach (var component in pathComponents)
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
  }
}