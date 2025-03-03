using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockDirectory : IDirectory
  {
    private readonly string _name;
    private readonly MockDirectory _parent;
    private List<MockDirectory> _subDirectories;
    private List<MockFile> _files;
    private bool _exists;

    /// <summary>
    /// Constructs a volume root directory
    /// </summary>
    /// <param name="path"></param>
    public MockDirectory(string path)
    {
      _parent = null;
      _name = path;
      _exists = true;
      _subDirectories = new List<MockDirectory>();
      _files = new List<MockFile>();
    }

    /// <summary>
    /// Constructs a directory that is relative to the supplied parent
    /// </summary>
    /// <param name="parent"></param>
    /// <param name="path"></param>
    public MockDirectory(MockDirectory parent, string path)
    {
      _parent = parent;
      _name = path.Substring(path.LastIndexOf('\\') + 1);
      _exists = true;
      _subDirectories = new List<MockDirectory>();
      _files = new List<MockFile>();
    }

    public MockDirectory(MockDirectory parent, string path, bool exists)
    {
      _parent = parent;
      _name = path.Substring(path.LastIndexOf('\\') + 1);
      _subDirectories = new List<MockDirectory>();
      _files = new List<MockFile>();
      _exists = exists;
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
        IDirectory directory = this;
        while (directory != null)
        {
          sb.Insert(0, directory.Name + @"\");
          directory = directory.Parent;
        }

        // strip the tailing slash if not at root and return
        string fullName = sb.Remove(sb.Length - 1, 1).ToString();
        if (fullName.Contains(@"\")) return fullName;
        else return sb.Append(@"\").ToString();
      }
    }

    public List<IFile> GetFiles(string fileSpec)
    {
      AssertExists();

      var foundFiles = new List<IFile>();

      foreach (IFile file in _files)
      {
        if (IsFileNameMatch(fileSpec, file.Name)) foundFiles.Add(file);
      }

      return foundFiles;
    }

    internal static bool IsFileNameMatch(string fileSpec, string fileName)
    {
      string pattern = fileSpec.Replace(".", @"\.").Replace("*", ".*").Replace("+", @"\+");
      if (!pattern.StartsWith(".*")) pattern = "^" + pattern;
      //if (!pattern.EndsWith(".*")) pattern = "$" + pattern;
      Regex regex = new Regex(pattern, RegexOptions.IgnoreCase);

      if (fileSpec == fileName) return true;

      Match match = regex.Match(fileName);
      return match.Success;
    }

    public IDirectory Parent
    {
      get { return _parent; }
    }

    public List<IDirectory> GetSubDirectories(string fileSpec)
    {
      AssertExists();

      var foundDirectories = new List<IDirectory>();

      foreach (MockDirectory directory in _subDirectories)
      {
        //if (directory.Name == fileSpec)
        foundDirectories.Add(directory);
      }

      return foundDirectories;
    }

    public void Create()
    {
      if (_parent == null) throw new DirectoryNotFoundException("Parent directory does not exist.");

      _parent.AddDirectory(_name);
      _exists = true;
    }

    public void Delete()
    {
      AssertExists();

      _subDirectories = new List<MockDirectory>();
      _files = new List<MockFile>();
      _exists = false;
    }

    public bool Exists
    {
      get { return _exists; }
    }

    public void CopyTo(string path)
    {
      AssertExists();
    }

    public void Empty(params string[] exclusions)
    {
      _files = new List<MockFile>();
      _subDirectories = new List<MockDirectory>();
    }

    public MockDirectory AddDirectory(string name)
    {
      AssertExists();

      var directory = new MockDirectory(this, name);
      _subDirectories.Add(directory);
      return directory;
    }

    public IFile AddFile(string name, string data)
    {
      AssertExists();
      return new MockFile(this, name, data);
    }

    public IFile AddFile(string name, byte[] data)
    {
      AssertExists();
      return new MockFile(this, name, data);
    }

    public void AddFile(MockFile file)
    {
      AssertExists();

      _files.Add(file);
    }

    private void AssertExists()
    {
      if (!_exists)
      {
        throw new DirectoryNotFoundException("Directory not found in test repository: " + FullName);
      }
    }
  }
}