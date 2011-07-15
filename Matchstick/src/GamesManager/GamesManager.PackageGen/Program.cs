using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using ICSharpCode.SharpZipLib.Zip;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model;
using MGDF.GamesManager.Model.Entities;
using Directory=System.IO.Directory;
using File=System.IO.File;

namespace MGDF.GamesManager.PackageGen
{
    class UpdateDetails
    {
        public string UpdateDescription;
        public Version UpdateVersion;
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

            if (args.Length==3 && args[1]=="-o")
            {
                try
                {
                    Console.WriteLine("Creating installer package...");

                    if (!Directory.Exists(args[0]))
                    {
                        Console.WriteLine("Error: Directory "+args[0]+" not found");
                        return 1;
                    }

                    CreateInstaller(args[0], args[2], file => file.StartsWith(".svn"));

                    if (!ValidateInstaller(args[2])) return 1;

                    Console.WriteLine("Installer package created.");
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Error: "+ex);
                    Console.WriteLine("Installer package creation failed.");
                }
            }
            else if (args.Length == 5 && args[0] == "-u" && args[3] == "-o")
            {
                try
                {
                    Console.WriteLine("Creating update installer package...");

                    if (!File.Exists(args[1]))
                    {
                        Console.WriteLine("Error: File " + args[1] + " not found");
                        return 1;
                    }

                    GameInstall oldInstall;
                    if (!ValidateNonUpdateInstaller(args[1],out oldInstall)) return 1;

                    if (!File.Exists(args[2]))
                    {
                        Console.WriteLine("Error: File " + args[2] + " not found");
                        return 1;
                    }

                    GameInstall newInstall;
                    if (!ValidateNonUpdateInstaller(args[2],out newInstall)) return 1;

                    var details = new UpdateDetails
                                      {
                                          UpdateDescription = newInstall.Game.Description,
                                          UpdateVersion = oldInstall.Game.Version
                                      };
                    CreateUpdateInstaller(details, args[1], args[2], args[4]);

                    if (!ValidateInstaller(args[4])) return 1;
                    
                    Console.WriteLine("Installer update package created.");
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Error: " + ex);
                    Console.WriteLine("Installer update package creation failed.");
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

        private static bool ValidateInstaller(string installerFile)
        {
            Console.WriteLine("Validating installer package "+installerFile+"...");
            GameInstall install = new GameInstall(installerFile);
            return ValidateInstaller(install);
        }

        private static bool ValidateNonUpdateInstaller(string installerFile,out GameInstall install)
        {
            Console.WriteLine("Validating installer package " + installerFile + "...");
            install = new GameInstall(installerFile);
            if (ValidateInstaller(install))
            {
                if (install.IsUpdate)
                {
                    Console.WriteLine("Error: Cannot use update installers as the basis for creating an update.");
                    return false;
                }
                return true;
            }
            return false;
        }

        private static bool ValidateInstaller(GameInstall install)
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
            return true;
        }

        public static void CreateInstaller(string input, string output,Func<string, bool> excludeFromCompression)
        {
            Console.WriteLine("Creating install package "+output+"...");

            if (!input.EndsWith("\\")) input += "\\";
            int trimOffset = (string.IsNullOrEmpty(input) ? Path.GetPathRoot(input).Length : input.Length);
 
            List<string> fileSystemEntries = new List<string>();
            fileSystemEntries.AddRange(Directory.GetDirectories(input, "*", SearchOption.AllDirectories).Select(d => d + "\\"));
            fileSystemEntries.AddRange(Directory.GetFiles(input, "*", SearchOption.AllDirectories));

            using (var outputStream = new FileStream(output, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                using (var compressor = new ZipOutputStream(outputStream))
                {
                    compressor.SetLevel(9);

                    foreach (var filePath in fileSystemEntries)
                    {
                        if (excludeFromCompression(filePath))
                        {
                            continue;
                        }

                        if (filePath.EndsWith("\\"))
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

                        var data = new byte[2048];
                        using (var inputStream = File.OpenRead(filePath))
                        {
                            int bytesRead;

                            while ((bytesRead = inputStream.Read(data, 0, data.Length)) > 0)
                            {
                                compressor.Write(data, 0, bytesRead);
                            }
                        }
                    }

                    compressor.Finish();
                }
            }
        }

        public static void CreateUpdateInstaller(UpdateDetails updateDetails, string oldArchiveFile, string newArchiveFile, string updateArchiveFile)
        {
            Console.WriteLine("Creating Update install package " + updateArchiveFile + "...");

            //list all files/directories in the old archive
            var oldDirectories = new Dictionary<string, ZipEntry>();
            var oldFiles = new Dictionary<string, ZipEntry>();
            var oldArchive = new ZipFile(oldArchiveFile);
            foreach (ZipEntry entry in oldArchive)
            {
                string name = entry.Name.ToLowerInvariant();
                if (entry.IsFile)
                {
                    oldFiles.Add(name, entry);
                }
                else
                {
                    oldDirectories.Add(name, entry);
                }
            }

            //list all files/directories in the new archive
            var newDirectories = new Dictionary<string, ZipEntry>();
            var newFiles = new Dictionary<string, ZipEntry>();
            var newArchive = new ZipFile(newArchiveFile);
            foreach (ZipEntry entry in newArchive)
            {
                string name = entry.Name.ToLowerInvariant();
                if (entry.IsFile)
                {
                    newFiles.Add(name, entry);
                }
                else
                {
                    newDirectories.Add(name, entry);
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
                string name = entry.Name.ToLowerInvariant();
                if (entry.IsFile)
                {
                    if (!oldFiles.ContainsKey(name) || oldFiles[name].DateTime < entry.DateTime)
                        addFiles.Add(name, entry);
                }
                else
                {
                    if (!oldDirectories.ContainsKey(name) || oldDirectories[name].DateTime < entry.DateTime)
                        addDirectories.Add(name, entry);
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
                    foreach (var file in addFiles)
                    {
                        compressor.PutNextEntry(new ZipEntry(file.Value.Name)
                                                    {
                                                        DateTime = file.Value.DateTime,
                                                        Size = file.Value.Size
                                                    });

                        var data = new byte[2048];
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

                    //add update.xml
                    byte[] updateData = CreateUpdateFileData(updateDetails, removeFiles);
                    ZipEntry updateEntry = new ZipEntry("update.xml")
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

        public static byte[] CreateUpdateFileData(UpdateDetails details, List<string> removeFiles)
        {
            XmlWriterSettings settings = new XmlWriterSettings
                                             {
                                                 OmitXmlDeclaration = false,
                                                 Indent = true,
                                                 NewLineChars = "\r\n",
                                                 IndentChars = "\t"
                                             };

            using (var stream = new MemoryStream())
            {
                XmlWriter writer = XmlWriter.Create(stream, settings);

                writer.WriteStartDocument();
                writer.WriteStartElement("mgdf", "update", "http://schemas.matchstickframework.org/2007/update");
                writer.WriteAttributeString("xmlns", "xsi", null, "http://www.w3.org/2001/XMLSchema-instance");

                writer.WriteElementString("updatedescription", details.UpdateDescription);
                writer.WriteElementString("updateminversion", details.UpdateVersion.ToString());
                writer.WriteElementString("updatemaxversion", details.UpdateVersion.ToString());

                writer.WriteStartElement("removefiles");
                foreach (var file in removeFiles)
                {
                    writer.WriteElementString("file", file);
                }
                writer.WriteEndElement();

                writer.WriteEndElement();
                writer.WriteEndDocument();
                writer.Close();

                return stream.ToArray();
            }
        }
    }
}