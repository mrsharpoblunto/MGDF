using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Entities;

namespace MGDF.GamesManager.Model
{
    public class GameDownloader : FileDownloader
    {
        private readonly Func<GetCredentialsEventArgs, bool> _newCredentials;
        private bool _requiresManualIntervention;
        private readonly Game _game;

        public GameDownloader(Game game,string sourceUrl, string destinationFileName, string expectedMd5, Func<GetCredentialsEventArgs, bool> getCredentials)
            : base(sourceUrl, destinationFileName, expectedMd5,null)
        {
            _game = game;
            //wrap the credentials call back in our own handler 
            //which first looks for stored credentials before prompting the user
            _getCredentials = GetCredentials;
            _newCredentials = getCredentials;
            _requiresManualIntervention = false;
        }

        public bool GetCredentials(GetCredentialsEventArgs args)
        {
            try
            {
                //check if this game has some stored details
                if (SettingsManager.Instance.Settings == null)
                {
                    //if not, create some
                    SettingsManager.Instance.Settings = new GameSettings { GameUid = _game.Uid, };
                }

                GetCredentialsEventArgs credentials = new GetCredentialsEventArgs
                                                          {
                                                              UserName = SettingsManager.Instance.Settings.UserName,
                                                              Password = SettingsManager.Instance.Settings.Password
                                                          };

                //if we have no credentials or we have incorrect stored credentials
                if (string.IsNullOrEmpty(credentials.UserName) || _requiresManualIntervention)
                {
                    if (_newCredentials(credentials))
                    {
                        SettingsManager.Instance.Settings.UserName = credentials.UserName;
                        SettingsManager.Instance.Settings.Password = credentials.Password;
                        SettingsManager.Instance.Save();
                        return true;
                    }
                    else
                    {
                        SettingsManager.Instance.Settings.UserName = string.Empty;
                        SettingsManager.Instance.Settings.Password = string.Empty;
                        SettingsManager.Instance.Save();
                        return false;
                    }
                }
                else
                {
                    credentials.UserName = SettingsManager.Instance.Settings.UserName;
                    credentials.Password = SettingsManager.Instance.Settings.Password;
                    return true;
                }
            }
            finally
            {
                //if this callback is called again, it must mean that the 
                //credentials supplied were wrong so must be entered manually
                _requiresManualIntervention = true;
            }
        }
    }
}