using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.Text;
using ACorns.WCF.DynamicClientProxy;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts.Entities;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.Model.ServiceModel
{
    public class StatisticsBundle: List<Statistic> {}

    public class StatisticsSession
    {
        public List<StatisticsBundle> Bundles { get; private set; }
        public Uri ServiceUrl { get; private set; }

        public StatisticsSession(string gameUid,string serviceUrl,string filename)
        {
            ServiceUrl = new Uri(serviceUrl,UriKind.Absolute);
            Bundles = new List<StatisticsBundle>();

            if (gameUid.Length > 255) return;

            var currentBundle = new StatisticsBundle();
            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Open,FileAccess.Read,FileShare.Read))
            {
                using (var reader = new StreamReader(stream))
                {
                    do
                    {
                        string line = reader.ReadLine();
                        int index = line.IndexOf(' ');

                        var stat = new Statistic
                                       {
                                           Name = line.Substring(0, index),
                                           Value = line.Substring(index + 1),
                                           GameUid = gameUid
                                       };
                        if (stat.Name.Length > 127) stat.Name = stat.Name.Substring(0, 127);
                        if (stat.Value.Length > 127) stat.Value = stat.Value.Substring(0, 127);
                        
                        currentBundle.Add(stat);
                        if (currentBundle.Count == 255)
                        {
                            Bundles.Add(currentBundle);
                            currentBundle = new StatisticsBundle();
                        }
                    } while (!reader.EndOfStream);
                    if (currentBundle.Count > 0) Bundles.Add(currentBundle);
                }
            }
        }
    }

    public class StatisticsServiceClient: IDisposable
    {
        private readonly IStatisticsService _service;
        private readonly StatisticsSession _session;

        public static Func<Uri, IStatisticsService> ServiceFactory = uri => WCFClientProxy<IStatisticsService>.GetReusableFaultUnwrappingInstance(new BasicHttpBinding(), new EndpointAddress(uri));

        public StatisticsServiceClient(StatisticsSession session)
        {
            _service = ServiceFactory(session.ServiceUrl);
            _session = session;
        }

        public void SendStatistics(List<string> errors)
        {
            try
            {
                foreach (var bundle in _session.Bundles)
                {
                    var request = new AddStatisticsRequest();

                    foreach (var statistic in bundle)
                    {
                        request.Statistics.Add(statistic);
                    }

                    var response = _service.AddStatistics(request);
                    if (response.Errors.Count > 0)
                    {
                        errors.AddRange(response.Errors);
                    }
                }

            }
            catch (Exception)
            {
                errors.Add("Unable to access StatisticsService at " + _session.ServiceUrl);
            }
        }

        public string Url
        {
            get { return _session.ServiceUrl.ToString(); }
        }

        public void Dispose()
        {
            (_service as IDisposable).Dispose();
        }
    }
}