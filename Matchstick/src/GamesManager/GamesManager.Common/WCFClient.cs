using System;
using System.Collections.Generic;
using System.Linq;
using System.ServiceModel;
using System.Text;

namespace MGDF.GamesManager.Common
{
    public interface IWCFClient<TChannel>
    {
        TReturn Use<TReturn>(Func<TChannel, TReturn> code);
    }

    public class WCFClient<TChannel> : IWCFClient<TChannel>
    {
        private readonly ChannelFactory<TChannel> _factory;

        public WCFClient(ChannelFactory<TChannel> factory)
        {
            _factory = factory;
        }

        public TReturn Use<TReturn>(Func<TChannel, TReturn> code)
        {
            TChannel channel = _factory.CreateChannel();
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
}
