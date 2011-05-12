using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.Text;
using System.Threading;
using ACorns.WCF.DynamicClientProxy;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Extensions;
using MGDF.GamesManager.Controls;
using MGDF.GamesManager.GameSource.Contracts;
using MGDF.GamesManager.GameSource.Contracts.Messages;
using MGDF.GamesManager.GameSource.DataLoader.MVP.Model;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View;
using MGDF.GamesManager.GameSource.DataLoader.MVP.View.Impl;
using MGDF.GamesManager.Model.Helpers;

namespace MGDF.GamesManager.GameSource.DataLoader.MVP.Presenter
{
    class LoginPresenter: DataLoaderPresenterBase<ILoginView>
    {
        public LoginPresenter()
        {
            View.OnLogin += View_OnLogin;
            View.Shown += View_Shown;

            View.DeveloperKey = Settings.Instance.DeveloperKey;
            View.SecretKey = Settings.Instance.SecretKey;
            View.GameSourceUrl= Settings.Instance.GameSource;
        }

        void View_Shown(object sender, EventArgs e)
        {
            if (!string.IsNullOrEmpty(View.GameSourceUrl))
            {
                View.ShowLoggingIn = true;
                DoLogin();
            }
            else
            {
                View.ShowLoggingIn = false;
            }
        }

        void View_OnLogin(object sender, EventArgs e)
        {
            View.ShowLoggingIn = true;
            DoLogin();
        }

        private void DoLogin()
        {
            if (string.IsNullOrEmpty(View.GameSourceUrl))
            {
                View.ShowError("Invalid GameSource URL");
                View.ShowLoggingIn = false;
            }

            Settings.Instance.DeveloperKey = View.DeveloperKey;
            Settings.Instance.SecretKey = View.SecretKey;
            Settings.Instance.GameSource = View.GameSourceUrl.TrimEnd(new[]{'/'});

            var loginThread = new Thread(DoLoginAsync);
            loginThread.Start();
        }

        private void DoLoginAsync()
        {
            try
            {
                GameSourceManifestHelper manifestHelper = new GameSourceManifestHelper(Settings.Instance.GameSource);
                IGameSourceDeveloperService developerService = WCFClientProxy<IGameSourceDeveloperService>.GetReusableFaultUnwrappingInstance(new BasicHttpBinding(), new EndpointAddress(manifestHelper.DeveloperServiceUrl));
                IGameSourceService gamesService = WCFClientProxy<IGameSourceService>.GetReusableFaultUnwrappingInstance(new BasicHttpBinding(), new EndpointAddress(manifestHelper.GamesServiceUrl));

                var response = developerService.CheckCredentials(RequestBuilder.Build<AuthenticatedRequestBase>());
                if (response.Errors.Count > 0)
                {
                    View.Invoke(()=>
                                    {
                                        View.ShowError("Invalid developer key/secret key");
                                        View.ShowLoggingIn = false;
                                    });
                }
                else
                {
                    Settings.Instance.Credentials = response.Developer;
                    ServiceLocator.Current.Register(developerService);
                    ServiceLocator.Current.Register(gamesService);
                    View.Invoke(() =>
                                    {
                                        if (View.RememberMe)
                                        {
                                            Settings.Instance.Save();
                                        }
                                        CloseView();
                                    });
                }
            }
            catch (Exception ex)
            {
                Logger.Current.Write(LogInfoLevel.Error, "Invalid GameSource URL: " + ex);
                View.Invoke(() =>
                                {
                                    View.ShowError("Invalid GameSource URL");
                                    View.ShowLoggingIn = false;
                                });
            }
        }
    }
}
