using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Runtime.CompilerServices;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.ServiceModel.Description;
using System.Text;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Entities;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Entities.XmlEntities;
using MGDF.GamesManager.Model.Events;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.Model.ClientModel
{
    public class GameSourceClient
    {
        //exposed to allow injecting mock service during unit tests.
        public static Func<string, IWCFClient<IGameSourceService>> ServiceFactory = gameSourceService =>
                                                                            {
                                                                                GameSourceServiceLocator serviceLocator = new GameSourceServiceLocator(gameSourceService);

                                                                                var factory = new ChannelFactory<IGameSourceService>(new WebHttpBinding(), new EndpointAddress(serviceLocator.GamesServiceUrl));
                                                                                factory.Endpoint.Behaviors.Add(new WebHttpBehavior());

                                                                                return new WCFClient<IGameSourceService>(factory);
                                                                            };

        public static GameSettings GetStatisticsPermission(IGame game, GameSettings sourceSettings, Func<GetStatsPermissionEventArgs, bool> getStatsPermission)
        {
            //check if this gamesource has some stored details
            if (sourceSettings == null)
            {
                //if not, create some
                sourceSettings = new GameSettings
                {
                    GameUid = game.Uid,
                };

                SettingsManager.Instance.Games.Add(sourceSettings);
            }

            if (!sourceSettings.StatisticsServiceEnabled.HasValue)
            {
                //request the users permission to gather gameplay stats
                if (!string.IsNullOrEmpty(game.StatisticsService))
                {
                    sourceSettings.StatisticsServiceEnabled = getStatsPermission(
                        new GetStatsPermissionEventArgs
                        {
                            Url = game.StatisticsService,
                            PrivacyPolicyUrl = game.StatisticsPrivacyPolicy
                        });
                }
                else
                {
                    sourceSettings.StatisticsServiceEnabled = false;
                }
                SettingsManager.Instance.Save();
            }

            return sourceSettings;
        }

        public List<GameVersionUpdate> GetGameUpdates(IGame game, Func<GetCredentialsEventArgs, bool> getCredentials, Func<GetStatsPermissionEventArgs, bool> getStatsPermission,List<string> errors)
        {
            var updates = new List<GameVersionUpdate>();

            if (!string.IsNullOrEmpty(game.GameSourceService))
            {
                try
                {
                    var gamesService = ServiceFactory(game.GameSourceService);
                    Version version = game.Version;

                    GameVersionUpdate latestVersion;
                    //get a contiguous chain of updates for the currently installed version
                    do
                    {
                        var response = gamesService.Use(s=>s.GetGameUpdate(Constants.InterfaceVersion.ToString(),game.Uid,version.ToString()));
                        if (response.Errors.Count != 0)
                        {
                            errors.AddRange(response.Errors.Map(e=>e.Code+": "+e.Message));
                            latestVersion = null;
                        }
                        else
                        {
                            latestVersion = response.LatestVersion;
                            if (latestVersion != null)
                            {
                                version = new Version(response.LatestVersion.Version);
                                updates.Add(response.LatestVersion);
                            }
                        }
                    } while (latestVersion != null);

                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex, "Error getting updates from gameService " + game.GameSourceService);
                    errors.Add("Unable to get updates from gameSource");
                }
            }
            else
            {
                errors.Add("Unable to update, No GameSource specified for this game.");
            }
            
            if (updates.Count>0)
            {
                try
                {
                    //check if this gamesource has some stored details
                    var sourceSettings = SettingsManager.Instance.Games.Find(g => g.GameUid == game.Uid);
                    sourceSettings = GetStatisticsPermission(game, sourceSettings, getStatsPermission);

                    //if the updates require authentication
                    if (updates[0].Game.RequiresAuthentication)
                    {
                        GetCredentialsEventArgs credentials = new GetCredentialsEventArgs
                                                                  {
                                                                      UserName = sourceSettings.UserName,
                                                                      Password = sourceSettings.Password
                                                                  };

                        if (string.IsNullOrEmpty(credentials.UserName))
                        {
                            if (!getCredentials(credentials))
                            {
                                errors.Add("No valid credentials supplied");
                                return new List<GameVersionUpdate>();
                            }
                        }

                        //check credentials and keep prompting until they are correct, or the user gives up
                        bool hasValidCredentials;
                        do
                        {
                            hasValidCredentials = HttpRequestManager.Current.HasValidCredentials(updates[0].DownloadURL, credentials.UserName, credentials.Password);
                        }
                        while (!hasValidCredentials && getCredentials(credentials));

                        if (!hasValidCredentials)
                        {
                            errors.Add("No valid credentials supplied");
                            return new List<GameVersionUpdate>();
                        }
                        else
                        {
                            //if valid credentials were entered, save them 
                            sourceSettings.UserName = credentials.UserName;
                            sourceSettings.Password = credentials.Password;
                            SettingsManager.Instance.Save();
                        }
                    }
                }
                catch (Exception ex)
                {
                    Logger.Current.Write(ex, "Error checking credentials " + game.GameSourceService);
                    errors.Add("Unable to check credentials");
                }
            }
            return updates;
        }
    }
}