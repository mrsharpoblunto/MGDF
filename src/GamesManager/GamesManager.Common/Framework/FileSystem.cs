using System.IO;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using Directory = MGDF.GamesManager.Common.Framework.Directory;
using File = MGDF.GamesManager.Common.Framework.File;

namespace MGDF.GamesManager.Common.Framework
{
  public interface IFileSystem
  {
    IDirectory GetDirectory(string path);
    IFile GetFile(string path);
    bool FileExists(string filename);
    bool DirectoryExists(string directory);
    bool UpdateFileLastAccess { get; set; }
  }

  public class FileSystem : IFileSystem
  {
    public static IFileSystem Current
    {
      get
      {
        return ServiceLocator.Current.Get<IFileSystem>();
      }
      set
      {
        ServiceLocator.Current.Register(value);
      }
    }

    public static string Combine(params string[] pathComponents)
    {
      if (pathComponents.Length > 1)
      {
        string result = pathComponents[0];
        for (int i = 1; i < pathComponents.Length; ++i)
        {
          if (pathComponents[i].StartsWith("\\") || pathComponents[i].StartsWith("/")) pathComponents[i] = pathComponents[i].Substring(1);
          result = Path.Combine(result, pathComponents[i]);
        }
        return result;
      }
      else if (pathComponents.Length == 1)
      {
        return pathComponents[0];
      }
      else
      {
        return null;
      }
    }

    public FileSystem()
    {
      UpdateFileLastAccess = true;
    }

    public FileSystem(bool updateFileLastAccess)
    {
      UpdateFileLastAccess = updateFileLastAccess;
    }

    public IDirectory GetDirectory(string path)
    {
      return new Directory(path);
    }

    public IFile GetFile(string path)
    {
      return new File(path);
    }

    public bool FileExists(string filename)
    {
      return new File(filename).Exists;
    }

    public bool DirectoryExists(string directory)
    {
      return new Directory(directory).Exists;
    }

    public bool UpdateFileLastAccess { get; set; }
  }
}