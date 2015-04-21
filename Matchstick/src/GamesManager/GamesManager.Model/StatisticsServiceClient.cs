using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceModel.Description;
using System.ServiceModel.Web;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts.Entities;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.Model
{
    public class StatisticsBundle: List<Statistic> {
		public StatisticsBundle(string gameUid,string sessionId) : base()
		{
			GameUid = gameUid;
			SessionId = sessionId;
		}

		public string SessionId { get; private set; }
		public string GameUid { get; private set; }
	}

    public class GetStatsPermissionEventArgs: EventArgs
    {
        public string Url;
        public string PrivacyPolicyUrl;
    }

    public class StatisticsSession
    {
        public List<StatisticsBundle> Bundles { get; private set; }
        public Uri ServiceUrl { get; private set; }

        public static bool CanSendStatistics(Game game)
        {
            //NOTE a privacy policy is required before the framework will let user data be sent
            return !string.IsNullOrEmpty(game.StatisticsService) && !string.IsNullOrEmpty(game.StatisticsPrivacyPolicy);
        }

        public static bool GetStatisticsPermission(Game game, Func<GetStatsPermissionEventArgs, bool> getStatsPermission)
        {
            //check if this game has some stored details
            if (SettingsManager.Instance.Settings == null)
            {
                //if not, create some
                SettingsManager.Instance.Settings = new GameSettings { GameUid = game.Uid, };
            }

            if (!SettingsManager.Instance.Settings.StatisticsServiceEnabled.HasValue)
            {
                //request the users permission to gather gameplay stats
                if (!string.IsNullOrEmpty(game.StatisticsService) && !string.IsNullOrEmpty(game.StatisticsPrivacyPolicy))
                {
                    SettingsManager.Instance.Settings.StatisticsServiceEnabled = getStatsPermission(
                        new GetStatsPermissionEventArgs
                        {
                            Url = game.StatisticsService,
                            PrivacyPolicyUrl = game.StatisticsPrivacyPolicy
                        });
                }
                else
                {
                    SettingsManager.Instance.Settings.StatisticsServiceEnabled = false;
                }
                SettingsManager.Instance.Save();
            }

            return SettingsManager.Instance.Settings.StatisticsServiceEnabled.Value;
        }

        public StatisticsSession(string gameUid,string serviceUrl,string filename)
        {
            ServiceUrl = new Uri(serviceUrl,UriKind.Absolute);
            Bundles = new List<StatisticsBundle>();

            if (gameUid.Length > 255) return;

			// correlate all the statistics in this session with a random id
			Guid sessionId = Guid.NewGuid();

            var currentBundle = new StatisticsBundle(gameUid,sessionId.ToString());
            using (var stream = FileSystem.Current.GetFile(filename).OpenStream(FileMode.Open,FileAccess.Read,FileShare.Read))
            {
                using (var reader = new StreamReader(stream))
                {
                    do
                    {
                        string line = reader.ReadLine().Trim();
						if (string.IsNullOrEmpty(line)) continue;

						int index = line.IndexOf(' ');
						string[] prefix = line.Substring(0,index).Split(new[]{':'});

                        var stat = new Statistic
                                       {
										   Timestamp = uint.Parse(prefix[0]),
                                           Name = prefix[1],
                                           Value = line.Substring(index + 1),
                                       };
                        if (stat.Name.Length > 255) stat.Name = stat.Name.Substring(0, 255);
                        if (stat.Value.Length > 255) stat.Value = stat.Value.Substring(0, 255);
                        
                        currentBundle.Add(stat);
                        if (currentBundle.Count == 255)
                        {
                            Bundles.Add(currentBundle);
                            currentBundle = new StatisticsBundle(gameUid,sessionId.ToString());
                        }
                    } while (!reader.EndOfStream);
                    if (currentBundle.Count > 0) Bundles.Add(currentBundle);
                }
            }
        }
    }

	public class WebWCFClient<TChannel> : IWCFClient<TChannel> where TChannel: class
	{
		public TReturn Use<TReturn>(Uri uri,Func<TChannel, TReturn> code)
		{
			var factory = new WebChannelFactory<TChannel>(uri);
			TChannel channel = factory.CreateChannel();
			bool error = true;
			try
			{
				TReturn result = code(channel);
				((IClientChannel)channel).Close();
				error = false;
				return result;
			}
			finally
			{
				if (error)
				{
					((IClientChannel)channel).Abort();
				}
			}
		}
	}

    public class StatisticsServiceClient
    {
        private readonly IWCFClient<IStatisticsService> _service;
        private readonly StatisticsSession _session;

        public static Func<IWCFClient<IStatisticsService>> ServiceFactory = ()=> new WebWCFClient<IStatisticsService>();

        public StatisticsServiceClient(StatisticsSession session)
        {
            _service = ServiceFactory();
            _session = session;
        }

        public void SendStatistics(List<string> errors)
        {
            try
            {
                foreach (var bundle in _session.Bundles)
                {
                    var request = new AddStatisticsRequest();
					request.GameUid = bundle.GameUid;
					request.SessionId = bundle.SessionId;
                    foreach (var statistic in bundle)
                    {
                        request.Statistics.Add(statistic);
                    }

                    var response = _service.Use(_session.ServiceUrl,s=>s.AddStatistics(request));
                    if (response.Errors.Count > 0)
                    {
                        errors.AddRange(response.Errors);
                    }
                }

            }
            catch (Exception ex)
            {
                errors.Add("Unable to access StatisticsService at " + _session.ServiceUrl+": "+ex.ToString());
            }
        }

        public string Url
        {
            get { return _session.ServiceUrl.ToString(); }
        }
    }
}