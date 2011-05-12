using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Model.Entities.XmlEntities.FragmentEntities
{
    abstract class PendingDownloadBase: PendingOperationBase
    {
        private const int DownloadBufferSize = 16384;

        protected string _sourceUrl;
        protected string _destinationFileName;

        protected PendingDownloadBase(XmlReader reader, ReaderWriterLockSlim readerWriterLock)
            : base(reader, readerWriterLock)
        {
        }

        protected PendingDownloadBase(ReaderWriterLockSlim readerWriterLock)
            : base(readerWriterLock)
        {
        }

        protected override void Work()
        {
            try
            {
                //For using untrusted SSL Certificates
                ServicePointManager.ServerCertificateValidationCallback += OnCheckRemoteCallback;

                long total;
                using (Stream responseSteam = HttpRequestManager.Current.GetResponseStream(_sourceUrl, Progress, out total))
                {
                    UpdateTotal(total);
                    using (Stream saveFileStream = FileSystem.Current.GetFile(_destinationFileName).OpenStream(FileMode.Append, FileAccess.Write, FileShare.ReadWrite))
                    {
                        int bytesRead;
                        byte[] downloadBuffer = new byte[DownloadBufferSize];

                        while ((bytesRead = responseSteam.Read(downloadBuffer, 0, downloadBuffer.Length)) > 0)
                        {
                            saveFileStream.Write(downloadBuffer, 0, bytesRead);
                            UpdateProgress(Progress +bytesRead);
                            if (PausePending || ErrorPending)
                            {
                                return;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Abort(string.Format("Unable to download {0} to {1} - {2}", _sourceUrl, _destinationFileName,ex));
            }
        }

        private static bool OnCheckRemoteCallback(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            return true;
        }
    }
}
