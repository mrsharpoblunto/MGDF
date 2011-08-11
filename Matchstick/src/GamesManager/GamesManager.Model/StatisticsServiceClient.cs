﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceModel.Description;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;
using MGDF.GamesManager.StatisticsService.Contracts;
using MGDF.GamesManager.StatisticsService.Contracts.Entities;
using MGDF.GamesManager.StatisticsService.Contracts.Messages;

namespace MGDF.GamesManager.Model
{
    public class StatisticsBundle: List<Statistic> {}

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

    public class StatisticsServiceClient
    {
        private readonly IWCFClient<IStatisticsService> _service;
        private readonly StatisticsSession _session;

        public static Func<Uri, IWCFClient<IStatisticsService>> ServiceFactory = uri =>
                                                                                     {
                                                                                         var factory = new ChannelFactory<IStatisticsService>(new WebHttpBinding(), new EndpointAddress(uri));
                                                                                         factory.Endpoint.Behaviors.Add(new WebHttpBehavior());

                                                                                         return new WCFClient<IStatisticsService>(factory);
                                                                                     };

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

                    var response = _service.Use(s=>s.AddStatistics(request));
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
    }
}