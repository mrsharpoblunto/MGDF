using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Xml;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;
using Newtonsoft.Json;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter
{
    class UploadGamePresenter: DataLoaderPresenterBase<IUploadGameVersionView>
    {
        public event EventHandler OnCancelled;
        public event EventHandler OnComplete;
        public event EventHandler OnError;

        private readonly object _lock = new object();
        private readonly string _gameFile;
        private readonly Guid _fragmentId;
        private readonly string _uploadHandler;
        private readonly int _maxUploadPartSize;
        private BackgroundWorker _worker;
        private bool _finished;

        public Guid FragmentId
        {
            get { return _fragmentId; }
        }

        public UploadGamePresenter(string gameFile, Guid fragmentId, string uploadHandler, int maxUploadPartSize)
        {
            _maxUploadPartSize = maxUploadPartSize;
            _gameFile = gameFile;
            _fragmentId = fragmentId;
            _uploadHandler = uploadHandler;
            View.OnCancel += View_OnCancel;
            View.Shown += View_Shown;
        }

        private void View_Shown(object sender, EventArgs e)
        {
            _worker = new BackgroundWorker {WorkerSupportsCancellation = true, WorkerReportsProgress = true};
            _worker.DoWork += Worker_DoWork;
            _worker.ProgressChanged += Worker_ProgressChanged;
            _worker.RunWorkerCompleted += Worker_RunWorkerCompleted;

            _worker.RunWorkerAsync();
        }

        private void Worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            View.Invoke(()=>View.Progress = e.ProgressPercentage);
        }

        private void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            IFile gameFile = FileSystem.Current.GetFile(_gameFile);

            if (gameFile.Exists)
            {
                try
                {
                    using (var input = gameFile.OpenStream(FileMode.Open, FileAccess.Read,FileShare.Read))
                    {
                        var buffer = new byte[_maxUploadPartSize];
                        int bytesRead = input.Read(buffer, 0, _maxUploadPartSize);
                        long completed = 0;
                        long total = gameFile.Length;

                        while (bytesRead>0)
                        {
                            completed += bytesRead;

                            int retryCount = 0;
                            bool succeeded = false;
                            do
                            {                             
                                if (_worker.CancellationPending)
                                {
                                    e.Cancel = true;
                                    e.Result = false;
                                    return;
                                }

                                DateTime now = TimeService.Current.Now;
                                string md5 = buffer.ComputeMD5(bytesRead);

                                var request = (HttpWebRequest)WebRequest.Create(_uploadHandler);
                                request.Timeout = int.MaxValue;
                                request.Method = "POST";
                                request.Headers.Add(Constants.Headers.DeveloperKey, Settings.Instance.DeveloperKey);
                                request.Headers.Add(Constants.Headers.Md5, md5);
                                Guid cnonce = Guid.NewGuid();
                                request.Headers.Add(Constants.Headers.CNonce, cnonce.ToString());//prevent replay attacks by requiring a single use unique token
                                request.Headers.Add(Constants.Headers.Timestamp, Cryptography.GenerateTimestamp(now));
                                request.Headers.Add(Constants.Headers.Hmac, Cryptography.GenerateHMac(Settings.Instance.SecretKey, cnonce+md5, now));//prevent mitm attacks from altering uploaded content by using the md5 as part of the hmac
                                request.Headers.Add(Constants.Headers.FragmentIdHeader, _fragmentId.ToString());
                                request.Headers.Add(Constants.Headers.IsLastFragment, (completed==total).ToString());

                                //write out the content to send
                                using (var requestStream = request.GetRequestStream())
                                {
                                    requestStream.Write(buffer, 0, bytesRead);
                                }

                                try
                                {
                                    request.GetResponse();
                                    succeeded = true;
                                    _worker.ReportProgress(Convert.ToInt32((completed / (double)total) * 100));
                                }
                                catch (WebException ex)
                                {
                                    Logger.Current.Write(LogInfoLevel.Error, "Unable to upload Game file: " + ex);
                                    var response = (HttpWebResponse)ex.Response;
                                    if (response.StatusCode == HttpStatusCode.Unauthorized)
                                    {
                                        View.Invoke(() => Message.Show("Authentication failed", "Unable to upload Game file"));
                                        e.Result = false;
                                        return;
                                    }
                                    else if (response.StatusCode == HttpStatusCode.BadRequest)
                                    {
                                        try
                                        {
                                            
                                            using (var reader = new StreamReader(response.GetResponseStream()))
                                            {
                                                var resultType = new 
                                                {
                                                    Success = false,
                                                    Code = string.Empty,
                                                    Message = string.Empty,
                                                    ShouldRetry = false
                                                };
                                                var result = JsonConvert.DeserializeAnonymousType(reader.ReadToEnd(), resultType);

                                                //the server could give us the opportunity to try uploading this part again if it failed due to an invalid part hash.
                                                if (!result.ShouldRetry)
                                                {
                                                    View.Invoke(() => Message.Show("Unable to upload Game file: " + result.Code+" - " + result.Message, "Unable to upload Game file"));
                                                    e.Result = false;                                                   
                                                }
                                            }
                                        }
                                        catch (Exception)
                                        {
                                            View.Invoke(() => Message.Show("Unable to upload Game file: Unknown error", "Unable to upload Game file"));
                                            e.Result = false;
                                            return;                                            
                                        }
                                    }
                                    else
                                    {
                                        View.Invoke(() => Message.Show("Unable to upload Game file: Unknown error", "Unable to upload Game file"));
                                        e.Result = false;
                                        return;
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Logger.Current.Write(LogInfoLevel.Error, "Unable to upload Game file: " + ex);
                                    View.Invoke(() => Message.Show("Unable to upload Game file: Unknown error", "Unable to upload Game file"));
                                    e.Result = false;
                                    return;
                                }
                            } while (!succeeded && retryCount++ < 3);

                            bytesRead = input.Read(buffer, 0, buffer.Length);
                        }
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(LogInfoLevel.Error, "Unable to upload Game file: " + ex);
                     View.Invoke(() => Message.Show(ex.Message, "Unable to upload Game file"));
                     e.Result = false;
                    return;                    
                }
            }
            else
            {
                Logger.Current.Write(LogInfoLevel.Error, "The specified Game file does not exist");
                 View.Invoke(() => Message.Show("The specified Game file does not exist","Game file not found"));
                 e.Result = false;
                return;
            }

            e.Result = true;
        }

        private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            lock (_lock)
            {
                if (!e.Cancelled)
                {
                    if ((bool)e.Result)
                    {
                        if (OnComplete != null)
                        {
                            OnComplete(this, new EventArgs());
                        }
                    }
                    else
                    {
                        if (OnError != null)
                        {
                            OnError(this, new EventArgs());
                        }
                    }
                }
                else
                {
                    if (OnCancelled != null)
                    {
                        OnCancelled(this, new EventArgs());
                    }
                }
                _finished = true;
            }

            View.Invoke(CloseView);
        }

        private void View_OnCancel(object sender, EventArgs e)
        {
            lock (_lock)
            {
                if (!_finished)
                {
                    _worker.CancelAsync();
                }
            }
        }
    }
}
