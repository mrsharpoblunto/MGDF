using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using ICSharpCode.SharpZipLib.Zip;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.Common.Extensions;
using Directory = System.IO.Directory;
using File = System.IO.File;
using Newtonsoft.Json;

namespace MGDF.GamesManager.PackageGen
{
  class UpdateDetails
  {
    public Version UpdateVersion;
  }

  class PackageMetadata
  {
    public Version Version;
    public string MD5;
  }

  class Program
  {
    //specify packagefile to create a new package
    //specify -u oldpackage newpackage to create an update from oldpackage to newpackage
    static int Main(string[] args)
    {
      FileSystem.Current = new FileSystem();
      TimeService.Current = new TimeService();
      ArchiveFactory.Current = new ArchiveFactory();

      if (args.Length == 3 && args[1] == "-o")
      {
        try
        {
          Console.WriteLine("Creating installer package");

          if (!Directory.Exists(args[0]))
          {
            Console.WriteLine("Error: Directory " + args[0] + " not found");
            return 1;
          }

          CreateInstaller(args[0], args[2], file => file.StartsWith(".svn", StringComparison.OrdinalIgnoreCase));

          PackageMetadata metadata = new PackageMetadata();
          if (!ValidateInstaller(args[2], metadata)) return 1;

          Console.WriteLine("Installer package created");
          Console.WriteLine("    Version: " + metadata.Version);
          Console.WriteLine("    MD5: " + metadata.MD5);
        }
        catch (Exception ex)
        {
          Console.WriteLine("Error: " + ex);
          Console.WriteLine("Installer package creation failed");
        }
      }
      else if (args.Length == 5 && args[0] == "-u" && args[3] == "-o")
      {
        try
        {
          Console.WriteLine("Creating update installer package");

          if (!File.Exists(args[1]))
          {
            Console.WriteLine("Error: File " + args[1] + " not found");
            return 1;
          }

          GameInstall oldInstall;
          if (!ValidateNonUpdateInstaller(args[1], out oldInstall)) return 1;

          if (!File.Exists(args[2]))
          {
            Console.WriteLine("Error: File " + args[2] + " not found");
            return 1;
          }

          GameInstall newInstall;
          if (!ValidateNonUpdateInstaller(args[2], out newInstall)) return 1;

          var details = new UpdateDetails
          {
            UpdateVersion = oldInstall.Game.Version
          };

          oldInstall.Dispose();
          newInstall.Dispose();

          CreateUpdateInstaller(details, args[1], args[2], args[4]);

          PackageMetadata metadata = new PackageMetadata();
          if (!ValidateInstaller(args[4], metadata)) return 1;

          Console.WriteLine("Installer update package created");
          Console.WriteLine("    Version: " + metadata.Version);
          Console.WriteLine("    MD5: " + metadata.MD5);
        }
        catch (Exception ex)
        {
          Console.WriteLine("Error: " + ex);
          Console.WriteLine("Installer update package creation failed");
        }
      }
      else if (args.Length == 0)
      {
        Console.WriteLine("Usage");
        Console.WriteLine();
        Console.WriteLine("GamesManager.PackageGen <installer package folder> -o <installer package>");
        Console.WriteLine("    Creates a new installer package from a directory.");
        Console.WriteLine("GamesManager.PackageGen -u <old installer package> <new installer package> -o <installer package>");
        Console.WriteLine("    Creates a new installer package that updates the old game to the new one.");
      }
      return 0;
    }

    private static bool ValidateInstaller(string installerFile, PackageMetadata data)
    {
      Console.WriteLine("Validating installer package " + installerFile);
      using (GameInstall install = new GameInstall(installerFile))
      {
        using (Stream stream = new FileStream(installerFile, FileMode.Open, FileAccess.Read, FileShare.Read))
        {
          data.MD5 = stream.ComputeMD5();
        }
        return ValidateInstaller(install, data);
      }
    }

    private static bool ValidateNonUpdateInstaller(string installerFile, out GameInstall install)
    {
      Console.WriteLine("Validating installer package " + installerFile);
      install = new GameInstall(installerFile);
      if (ValidateInstaller(install, null))
      {
        if (install.IsUpdate)
        {
          Console.WriteLine("Error: Cannot use update installers as the basis for creating an update.");
          install.Dispose();
          return false;
        }
        return true;
      }
      install.Dispose();
      return false;
    }

    private static bool ValidateInstaller(GameInstall install, PackageMetadata data)
    {
      if (install.ErrorCollection.Count > 0)
      {
        foreach (var error in install.ErrorCollection)
        {
          Console.WriteLine("Error: " + error);
        }
        Console.WriteLine("Error: Installer package creation failed.");
        return false;
      }
      else if (data != null)
      {
        data.Version = install.Game.Version;
      }
      return true;
    }

    public static void CreateInstaller(string input, string output, Func<string, bool> excludeFromCompression)
    {
      Console.Write("Creating install package " + output + " ");

      if (!input.EndsWith("\\", StringComparison.OrdinalIgnoreCase)) input += "\\";
      int trimOffset = (string.IsNullOrEmpty(input) ? Path.GetPathRoot(input).Length : input.Length);

      List<string> fileSystemEntries = new List<string>();
      fileSystemEntries.AddRange(Directory.GetDirectories(input, "*", SearchOption.AllDirectories).Select(d => d + "\\"));
      fileSystemEntries.AddRange(Directory.GetFiles(input, "*", SearchOption.AllDirectories));


      using (var outputStream = new FileStream(output, FileMode.Create, FileAccess.Write, FileShare.None))
      {
        using (var compressor = new ZipOutputStream(outputStream))
        {
          compressor.SetLevel(9);
          var data = new byte[4194304];
          int milestone = fileSystemEntries.Count / 10;
          int i = 0;

          foreach (var filePath in fileSystemEntries)
          {
            if (i++ % milestone == 0)
            {
              Console.Write('.');
            }

            if (excludeFromCompression(filePath))
            {
              continue;
            }

            if (filePath.EndsWith("\\", StringComparison.OrdinalIgnoreCase))
            {
              var directoryInfo = new DirectoryInfo(filePath);
              compressor.PutNextEntry(new ZipEntry(filePath.Substring(trimOffset))
              {
                DateTime = directoryInfo.LastWriteTimeUtc
              });
              continue;
            }
            else
            {
              var fileInfo = new FileInfo(filePath);
              compressor.PutNextEntry(new ZipEntry(filePath.Substring(trimOffset))
              {
                DateTime = fileInfo.LastWriteTimeUtc,
                Size = fileInfo.Length
              });
            }

            using (var inputStream = File.OpenRead(filePath))
            {
              int bytesRead;
              while ((bytesRead = inputStream.Read(data, 0, data.Length)) > 0)
              {
                compressor.Write(data, 0, bytesRead);
              }
            }
          }
          Console.WriteLine();
          compressor.Finish();
        }
      }
    }

    public static void CreateUpdateInstaller(UpdateDetails updateDetails, string oldArchiveFile, string newArchiveFile, string updateArchiveFile)
    {
      Console.Write("Creating Update install package " + updateArchiveFile + " ");

      //list all files/directories in the old archive
      var oldDirectories = new Dictionary<string, ZipEntry>();
      var oldFiles = new Dictionary<string, ZipEntry>();
      using (var oldArchive = new ZipFile(oldArchiveFile))
      {
        foreach (ZipEntry entry in oldArchive)
        {
          if (entry.IsFile)
          {
            oldFiles.Add(entry.Name, entry);
          }
          else
          {
            oldDirectories.Add(entry.Name, entry);
          }
        }


        //list all files/directories in the new archive
        var newDirectories = new Dictionary<string, ZipEntry>();
        var newFiles = new Dictionary<string, ZipEntry>();
        using (var newArchive = new ZipFile(newArchiveFile))
        {
          foreach (ZipEntry entry in newArchive)
          {
            if (entry.IsFile)
            {
              newFiles.Add(entry.Name, entry);
            }
            else
            {
              newDirectories.Add(entry.Name, entry);
            }
          }

          //any files in the original archive and not in the new one should be marked for removal.
          List<string> removeFiles = new List<string>();
          foreach (var dir in oldDirectories)
          {
            if (!newDirectories.ContainsKey(dir.Key)) removeFiles.Add(dir.Key);
          }
          foreach (var file in oldFiles)
          {
            if (!newFiles.ContainsKey(file.Key)) removeFiles.Add(file.Key);
          }

          //list all new or newer files/directories in archive2
          var addDirectories = new Dictionary<string, ZipEntry>();
          var addFiles = new Dictionary<string, ZipEntry>();
          foreach (ZipEntry entry in newArchive)
          {
            if (entry.IsFile)
            {
              if (!oldFiles.ContainsKey(entry.Name))
              {
                addFiles.Add(entry.Name, entry);
              }
              else
              {
                //do a md5hash of both files to determine if the content is different
                string oldHash;
                using (var stream = oldArchive.GetInputStream(oldFiles[entry.Name]))
                {
                  oldHash = stream.ComputeMD5();
                }
                string newHash;
                using (var stream = newArchive.GetInputStream(newFiles[entry.Name]))
                {
                  newHash = stream.ComputeMD5();
                }

                //if the new file has different content, then we want to include it
                if (oldHash != newHash)
                {
                  addFiles.Add(entry.Name, entry);
                }
              }
            }
            else
            {
              addDirectories.Add(entry.Name, entry);
            }
          }

          //write out the files to a new zip archive
          using (var outputStream = new FileStream(updateArchiveFile, FileMode.Create, FileAccess.Write, FileShare.None))
          {
            using (var compressor = new ZipOutputStream(outputStream))
            {
              compressor.SetLevel(9);

              //add new directories
              foreach (var dir in addDirectories)
              {
                compressor.PutNextEntry(new ZipEntry(dir.Value.Name)
                {
                  DateTime = dir.Value.DateTime,
                });
              }

              //add new or updated files
              var data = new byte[4194304];
              int milestone = Math.Max(1, addFiles.Count / 10);
              int i = 0;
              foreach (var file in addFiles)
              {
                if (i++ % milestone == 0)
                {
                  Console.Write('.');
                }
                compressor.PutNextEntry(new ZipEntry(file.Value.Name)
                {
                  DateTime = file.Value.DateTime,
                  Size = file.Value.Size
                });

                using (var inputStream = newArchive.GetInputStream(file.Value))
                {
                  int bytesRead = inputStream.Read(data, 0, data.Length);
                  while (bytesRead > 0)
                  {
                    compressor.Write(data, 0, bytesRead);
                    bytesRead = inputStream.Read(data, 0, data.Length);
                  }
                }
              }

              //add update.json
              byte[] updateData = CreateUpdateFileData(updateDetails, removeFiles);
              ZipEntry updateEntry = new ZipEntry("update.json")
              {
                DateTime = DateTime.UtcNow,
                Size = updateData.Length
              };
              compressor.PutNextEntry(updateEntry);
              compressor.Write(updateData, 0, updateData.Length);

              compressor.Finish();
            }
          }
        }
      }
      Console.WriteLine();
    }

    public static byte[] CreateUpdateFileData(UpdateDetails details, List<string> removeFiles)
    {
      using (var stream = new MemoryStream())
      {
        using (var textWriter = new StreamWriter(stream))
        {
          using (JsonWriter writer = new JsonTextWriter(textWriter))
          {
            writer.Formatting = Formatting.Indented;
            writer.WriteStartObject();

            writer.WriteRequiredValue("updateminversion", details.UpdateVersion.ToString());
            writer.WriteRequiredValue("updatemaxversion", details.UpdateVersion.ToString());
            writer.WritePropertyName("removefiles");
            writer.WriteStartArray();
            foreach (var file in removeFiles)
            {
              writer.WriteValue(file);
            }
            writer.WriteEndArray();

            writer.WriteEndObject();
          }
        }

        return stream.ToArray();
      }
    }
  }
}