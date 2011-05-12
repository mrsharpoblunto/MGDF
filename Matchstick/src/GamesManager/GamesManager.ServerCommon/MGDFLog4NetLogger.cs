using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using log4net;
using log4net.Config;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.ServerCommon
{
    public class MGDFLog4NetLogger : ILogger
    {
        private readonly ILog _log;

        public MGDFLog4NetLogger()
        {
            XmlConfigurator.Configure();
            _log = LogManager.GetLogger("GlobalLogging");
        }

        public void Error(string message, Exception detail)
        {
            _log.Error(message, detail);
        }

        public void Write(LogInfoLevel level, string message)
        {
            switch (level)
            {
                case LogInfoLevel.Error:
                    _log.Error(message);
                    break;
                case LogInfoLevel.Info:
                    _log.Info(message);
                    break;
                case LogInfoLevel.Warning:
                    _log.Warn(message);
                    break;
            }
        }

        public void Write(Exception ex, string message)
        {
            _log.Error(message, ex);
        }
    }
}