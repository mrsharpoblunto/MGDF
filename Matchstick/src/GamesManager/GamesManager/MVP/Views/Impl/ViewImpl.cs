using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Controls;

namespace MGDF.GamesManager.MVP.Views.Impl
{
    class ViewImpl
    {
        public static void RegisterViews()
        {
            ViewFactory.Current.RegisterView<IGetCredentialsView,GetCredentialsView>();
            ViewFactory.Current.RegisterView<IGetUpdatePermissionView, GetUpdatePermissionView>();
            ViewFactory.Current.RegisterView<IProgressView, ProgressView>();
            ViewFactory.Current.RegisterView<ISendStatisticsView, SendStatisticsView>();
            ViewFactory.Current.RegisterView<ISubmitErrorEmailView, SubmitErrorEmailView>();
            ViewFactory.Current.RegisterView<ISubmitErrorS3View, SubmitErrorS3View>();
        }
    }
}