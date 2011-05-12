using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Events;
using IArchiveFile=MGDF.GamesManager.Model.Factories.IArchiveFile;

namespace MGDF.GamesManager.Model.Helpers
{
	class SubTreeData {
		public int FileCount{ get; set;}
		public long BytesCount{ get; set;}
	}


    class ArchiveFileHelper
    {
        private const int BUFFER_SIZE = 65536;
        private const int PROGRESS_INCREMENT_SIZE = 1048576;

        public event EventHandler<CopyProgressEventArgs> OnCopyProgress;
        public event EventHandler OnCancelled;

        public static SubTreeData GetSubTreeData(IArchiveFile vfsSubtree)
        {
            var result = new SubTreeData();

            if (vfsSubtree != null)
            {
                GetSubTreeDataRecursive(vfsSubtree, result);
            }
            return result;
        }

        private static void GetSubTreeDataRecursive(IArchiveFile vfsSubtree, SubTreeData data)
        {
            if (vfsSubtree.IsFolder) {
                foreach (var file in vfsSubtree.Children.Values) {
                    GetSubTreeDataRecursive(file, data);
	            }
            }
            else {
                data.FileCount++;
                data.BytesCount += vfsSubtree.Size;
            }		
        }

        public void CopyVfsSubtree(IArchiveFile subTree, string baseDirectory, bool excludeRootNode)
        {
            if (subTree == null) return;

	        if (FileSystem.Current.DirectoryExists(baseDirectory)) {
		        if (excludeRootNode) {
                    foreach (var file in subTree.Children.Values)
                    {
                        bool cancel = CopyVfsSubtreeRecursive(file,baseDirectory);
                        if (cancel) break;
                    }
		        }
		        else 
		        {
                    CopyVfsSubtreeRecursive(subTree, baseDirectory);
		        }
	        }
	        else {
		        throw new Exception("base directory does not exist");
	        }
        }

        private bool CopyVfsSubtreeRecursive(IArchiveFile vfsSubtree, string baseDirectory)
        {
            if (vfsSubtree.IsFolder)
            {
                var directory = FileSystem.Current.GetDirectory(baseDirectory + "/" + vfsSubtree.Name);
                directory.Create();

                foreach (var file in vfsSubtree.Children.Values)
                {
                    bool cancel = CopyVfsSubtreeRecursive(file, baseDirectory + "/" + vfsSubtree.Name);
                    if (cancel) return true;
                }
            }
            else
            {
                string filename = baseDirectory + "/" + vfsSubtree.Name;
                return CopyVfsFileInternal(vfsSubtree, filename);
            }

            return false;
        }

        public void CopyVfsFile(IArchiveFile vfsFile, string filename)
        {
            if (vfsFile == null) return;

            CopyVfsFileInternal(vfsFile, filename);
        }

        private bool CopyVfsFileInternal(IArchiveFile vfsFile, string filename)
        {
            CopyProgressEventArgs args = new CopyProgressEventArgs
            {
                CurrentFileSize = vfsFile.Size,
                CurrentFile = vfsFile.Name,
                BytesCopied = 0,
                TotalBytesCopied = 0
            };

            using (Stream inStream = vfsFile.OpenStream())
            {
                using (Stream outStream = FileSystem.Current.GetFile(filename).OpenStreamWithTimeout(FileMode.Create,FileAccess.Write,FileShare.None))
                {
                    long bytesRemaining = vfsFile.Size;
                    byte[] buffer = new byte[BUFFER_SIZE];

                    while (bytesRemaining > 0)
                    {
                        int bytesRead = (int)(bytesRemaining < BUFFER_SIZE ? bytesRemaining : BUFFER_SIZE);

                        inStream.Read(buffer, 0, bytesRead);
                        outStream.Write(buffer, 0, bytesRead);
                        bytesRemaining -= bytesRead;

                        args.TotalBytesCopied += bytesRead;
                        args.BytesCopied += bytesRead;
                        if (OnCopyProgress != null && (args.BytesCopied % PROGRESS_INCREMENT_SIZE == 0))
                        {
                            OnCopyProgress(this, args);
                            args.BytesCopied = 0;
                            if (args.Cancel)
                            {
                                Cancel();
                                return true;
                            }
                        }
                    }
                }
            }

            if (OnCopyProgress != null)
            {
                OnCopyProgress(this, args);
            }

            return false;
        }

        private void Cancel()
        {
            if (OnCancelled!=null)
            {
                OnCancelled(this,new EventArgs());
            }
        }
    }
}
