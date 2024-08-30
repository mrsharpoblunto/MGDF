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

namespace MGDF.GamesManager.Model
{
  public class GetStatsPermissionEventArgs : EventArgs
  {
    public string Url;
    public string PrivacyPolicyUrl;
  }

  public class StatisticsSession
  {
    public static bool GetStatisticsPermission(Game game, Func<GetStatsPermissionEventArgs, bool> getStatsPermission)
    {
      //NOTE a privacy policy is required before the framework will let user data be sent
      if (string.IsNullOrEmpty(game.StatisticsService))
      {
        return false;
      }

      if (string.IsNullOrEmpty(game.StatisticsPrivacyPolicy))
      {
        Logger.Current.Write(LogInfoLevel.Warning, "Cannot send statistics due to missing privacy policy url");
        return false;
      }

      //check if this game has some stored details
      if (SettingsManager.Instance.Settings == null)
      {
        //if not, create some
        SettingsManager.Instance.Settings = new GameSettings { GameUid = game.Uid, };
      }

      if (!SettingsManager.Instance.Settings.StatisticsServiceEnabled.HasValue)
      {
        //request the users permission to gather gameplay stats
        SettingsManager.Instance.Settings.StatisticsServiceEnabled = getStatsPermission(
            new GetStatsPermissionEventArgs
            {
              Url = game.StatisticsService,
              PrivacyPolicyUrl = game.StatisticsPrivacyPolicy
            });
        SettingsManager.Instance.Save();
      }

      return SettingsManager.Instance.Settings.StatisticsServiceEnabled.Value;
    }
  }
}