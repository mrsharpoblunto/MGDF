﻿using System.Collections.Generic;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.ServiceModel.Description;
using System.ComponentModel;

namespace ACorns.WCF.DynamicClientProxy
{
	[EditorBrowsable(EditorBrowsableState.Never)]
	public abstract class WCFReusableFaultWrapperChannelProxy<T> : WCFReusableFaultWrapperClientProxy<T> where T : class
	{
		private static readonly Dictionary<string, ChannelFactory<T>> factoryCache = new Dictionary<string, ChannelFactory<T>>();
		private ChannelFactory<T> factory;

        protected WCFReusableFaultWrapperChannelProxy(Binding binding, EndpointAddress endpointAddress)
            : base(binding, endpointAddress)
		{
		}

		protected override T CreateProxyInstance()
		{
			AssureFactory();

			return factory.CreateChannel();
		}

		private void AssureFactory()
		{
			if (factory == null)
			{
				lock (factoryCache)
				{
					if (!factoryCache.TryGetValue(Binding.ToString()+EndpointAddress, out factory))
					{
                        factory = new ChannelFactory<T>(Binding.ToString() + EndpointAddress);
                        factoryCache[Binding.ToString() + EndpointAddress] = factory;
					}
				}
			}
		}

		public override ClientCredentials ClientCredentials
		{
			get
			{
				AssureProxy();
				return base.ClientCredentials;
			}
		}

		public override ServiceEndpoint CurrentEndpoint
		{
			get
			{
				if (manuallyClosed)
					return null;
				else
				{
					if (factory == null)
						return null;
					else
						return factory.Endpoint;
				}
			}
		}

		public override ServiceEndpoint Endpoint
		{
			get
			{
				AssureFactory();
				return factory.Endpoint;
			}
		}

		public override IClientChannel InnerChannel
		{
			get { return null; }
		}
	}
}