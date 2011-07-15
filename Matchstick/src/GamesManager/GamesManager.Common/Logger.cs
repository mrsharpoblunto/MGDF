using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Common
{
    public enum LogInfoLevel
    {
        Info,
        Warning,
        Error
    }

    public interface ILogger
    {
        void Write(LogInfoLevel level, string message);
        void Write(Exception ex, string message);
    }

    public class Logger: ILogger
    {
        private static readonly object _lock = new object();
        private readonly FileInfo _logFile;

        public static ILogger Current
        {
            get
            {
                return ServiceLocator.Current.Get<ILogger>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public Logger(string logFile)
        {
            _logFile = new FileInfo(logFile);

            //clear out the log file before we begin this session.
            try
            {
                using (new FileStream(_logFile.FullName, FileMode.Create, FileAccess.Write))
                {
                }
            }
            catch (Exception)
            {

            }
        }


        public void Write(LogInfoLevel level,string message)
        {
            lock (_lock)
            {
                try
                {
                    using (var fs = new FileStream(_logFile.FullName, FileMode.Append, FileAccess.Write))
                    {
                        using (TextWriter tw = new StreamWriter(fs, Encoding.UTF8))
                        {
                            tw.Write(DateTime.Now.ToShortTimeString() + " " + level + ": " + message+"\r\n");
                        }
                    }
                }
                catch (Exception)
                {
                    
                }
            }
        }

        public void Write(Exception ex,string message)
        {
            lock (_lock)
            {
                try 
                {
                    using (var fs = new FileStream(_logFile.FullName, FileMode.Append, FileAccess.Write))
                    {
                        using (TextWriter tw = new StreamWriter(fs, Encoding.UTF8))
                        {
                            tw.Write(DateTime.Now.ToShortTimeString() + " " + LogInfoLevel.Error + ": " + message + "\r\n");
                            tw.Write(ex.ToString() + "\r\n");
                        }
                    }
                }
                catch (Exception)
                {
                    
                }
            }
        }
    }
}