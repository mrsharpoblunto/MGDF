using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Model.Services
{
    public interface IService
    {
        void Start();
        void WaitForStatus(ServiceControllerStatus status);
        void Stop();
    }

    public interface IServiceManager
    {
        IService GetService(string name);
    }

    class RealService: IService
    {
        private ServiceController _serviceController;

        public RealService(string name)
        {
            _serviceController = new ServiceController(name);
        }

        public void Start()
        {
            if  (_serviceController.Status.Equals(ServiceControllerStatus.Stopped) || _serviceController.Status.Equals(ServiceControllerStatus.StopPending))
            {
               _serviceController.Start();
            }  
        }

        public void WaitForStatus(ServiceControllerStatus status)
        {
            _serviceController.WaitForStatus(status);
        }

        public void Stop()
        {
            _serviceController.Stop();
        }
    }

    public class ServiceManager : IServiceManager
    {
        public static IServiceManager Current
        {
            get
            {
                return ServiceLocator.Current.Get<IServiceManager>();
            }
            set
            {
                ServiceLocator.Current.Register(value);
            }
        }

        public IService GetService(string name)
        {
            return new RealService(name);
        }
    }
}
