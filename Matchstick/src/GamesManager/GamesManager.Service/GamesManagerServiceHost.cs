using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.ServiceModel;
using System.ServiceProcess;
using System.Text;
using System.Threading;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts;
using MGDF.GamesManager.Model.ServiceModel;
using MGDF.GamesManager.Model.Services;

namespace MGDF.GamesManager.Service
{
    public partial class GamesManagerServiceHost : ServiceBase
    {
        private readonly GamesManagerServiceContainer _container;

        public GamesManagerServiceHost()
        {
            InitializeComponent();
            CanShutdown = true;
            CanPauseAndContinue = false;
            CanStop = true;
            CanHandlePowerEvent = true;

            _container = new GamesManagerServiceContainer(true);
        }

        protected override void OnStart(string[] args)
        {
            _container.OnStart(args);
        }

        protected override bool OnPowerEvent(PowerBroadcastStatus powerStatus)
        {
            if (!_container.OnPowerEvent(powerStatus))
            {
                base.OnPowerEvent(powerStatus);
            }
            return true;
        }

        protected override void OnShutdown()
        {
            _container.OnShutdown();
            base.OnShutdown();
        }

        protected override void OnStop()
        {
            _container.OnStop();
            base.OnStop();
        }
    }
}