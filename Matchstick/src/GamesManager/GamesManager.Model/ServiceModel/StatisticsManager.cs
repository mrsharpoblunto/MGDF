using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model.ServiceModel
{
    class StatisticsManager: IDisposable
    {
        private readonly List<StatisticsSession> _pendingStatistics;
        private readonly Thread _statisticsThread;

        public StatisticsManager()
        {
            _pendingStatistics = new List<StatisticsSession>();
            _statisticsThread = new Thread(SendPendingStatistics);
            _statisticsThread.Start();
        }

        public void Dispose()
        {
            _statisticsThread.Abort();
            _statisticsThread.Join();
        }

        private void SendPendingStatistics()
        {
            try
            {
                while (true)
                {
                    var session = NextStatisticsSession();
                    while (session != null)
                    {
                        var client = new StatisticsServiceClient(session);
                        var errors = new List<string>();
                        client.SendStatistics(errors);
                        foreach (var error in errors)
                        {
                            Logger.Current.Write(LogInfoLevel.Error, error);
                        }

                        session = NextStatisticsSession();
                    }
                    Thread.Sleep(1000);
                }
            }
            catch (ThreadAbortException)
            {
            }
            catch (Exception ex)
            {
                Logger.Current.Write(ex,"Unexpected error sending pending statistics");                
            }
        }

        private StatisticsSession NextStatisticsSession()
        {
            lock (_pendingStatistics)
            {
                if (_pendingStatistics.Count > 0)
                {
                    var top = _pendingStatistics[0];
                    _pendingStatistics.RemoveAt(0);
                    return top;
                }
            }
            return null;
        }

        public void SendStatisticsSession(StatisticsSession session)
        {
            lock (_pendingStatistics)
            {
                _pendingStatistics.Add(session);
            }
        }
    }
}
