#pragma once

#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
This class represents the display settings for a particular adaptor mode
\author gcconner
*/
class IGraphicsAdaptorMode {
public:
	virtual UINT32  GetWidth() const=0;
	virtual UINT32  GetHeight() const=0;
	virtual UINT32 GetRefreshRateNumerator() const=0;
	virtual UINT32 GetRefreshRateDenominator() const=0;
};

DECLARE_LIST(IGraphicsAdaptorModeList,IGraphicsAdaptorMode *)
DECLARE_LIST(IUIntList,UINT32)

/**
this class allows you to get and set the engines various display settings
\author gcconner
*/
class IGraphicsManager {
public:

/**
gets the current vsync setting 
*/
virtual bool GetVSync() const=0;

/**
sets the current vsync setting 
*/
virtual void SetVSync(bool vsync)=0;

/**
returns the available multisample levels supported by the display adaptor
*/
virtual IUIntList *GetMultiSampleLevels() const=0;

/**
set the display adaptors current multisample level, this changed setting is not applied until Queue	ResetDevice is called.
\return returns false if the desired multisample level cannot be set.
*/
virtual bool SetBackBufferMultiSampleLevel(UINT32 multisampleLevel)=0;

/**
get the current multisample level in use by the adaptor
*/
virtual UINT32  GetBackBufferMultiSampleLevel() const=0;

/**
set the desired multisample level for off screen render targets. This setting is not used directly
by the framework but any client code should query this property when creatign render targets that
may require multisampling (see also GetCurrentMultiSampleLevel)
\return returns false if the desired multisample level cannot be set.
*/
virtual bool SetCurrentMultiSampleLevel(UINT32 multisampleLevel)=0;

/**
get the current desired multisample level for off screen render targets
\param quality if specified this parameter will be initialized with the maximum 
multisampling quality setting for the current multisample level
*/
virtual UINT32  GetCurrentMultiSampleLevel(UINT32 *quality) const=0;

/**
get a list of available adaptor modes
*/
virtual const IGraphicsAdaptorModeList * GetAdaptorModes() const=0;

/**
get the adaptor mode (if any) matching the requested width and height, if no matching adaptor is found, nullptr is returned
*/
virtual const IGraphicsAdaptorMode * GetAdaptorMode(UINT32 width,UINT32 height) const=0;

/**
get the current adaptor mode being used
*/
virtual const IGraphicsAdaptorMode * GetCurrentAdaptorMode() const=0;

/**
sets the current display adaptor mode, this changed setting is not applied until QueueResetDevice is called.
*/
virtual void  SetCurrentAdaptorMode(IGraphicsAdaptorMode *mode)=0;

/**
get the current screen width, based on the current adaptor mode
*/
virtual UINT32  GetScreenX() const=0;

/**
get the current screen height, based on the current adaptor mode
*/
virtual UINT32  GetScreenY() const=0;

/**
Queues the swap chain to be reset on the beginning of the next frame. This applies any changed adaptor mode or multisample settings.
After the changes are applied, the modules OnReset event will be fired.
*/
virtual void  ApplyChanges()=0;

/**
get the direct3d device object from the system
\return the direct3d device object from the system
*/
virtual ID3D11Device * GetD3DDevice() const=0;

/**
Gets the current back buffer texture. The pointer returned by this method becomes invalid when the modules OnReset event is fired
*/
virtual ID3D11Texture2D * GetBackBuffer() const=0;

/**
Gets the description of the backbuffer texture
*/
virtual void GetBackBufferDescription(D3D11_TEXTURE2D_DESC *desc) const=0;
};

}