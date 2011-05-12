#pragma once

#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
This class represents the display settings for a particular adaptor mode
\author gcconner
*/
class IGraphicsAdaptorMode {
public:
	virtual unsigned int  GetWidth() const=0;
	virtual unsigned int  GetHeight() const=0;
	virtual unsigned int GetRefreshRate() const=0;
};

DECLARE_LIST(IGraphicsAdaptorModeList,IGraphicsAdaptorMode *)

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
virtual unsigned int  GetMultiSampleLevels() const=0;

/**
set the display adaptors current multisample level, this changed setting is not applied until Queue	ResetDevice is called.
\return returns false if the desired multisample level cannot be set.
*/
virtual void  SetCurrentMultiSampleLevel(unsigned int multisampleLevel)=0;

/**
get the current multisample level in use by the adaptor
*/
virtual unsigned int  GetCurrentMultiSampleLevel() const=0;

/**
get a list of available adaptor modes
*/
virtual const IGraphicsAdaptorModeList * GetAdaptorModes() const=0;

/**
get the adaptor mode (if any) matching the requested width and height, if no matching adaptor is found, NULL is returned
*/
virtual const IGraphicsAdaptorMode * GetAdaptorMode(unsigned int width,unsigned int height) const=0;

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
virtual unsigned int  GetScreenX() const=0;

/**
get the current screen height, based on the current adaptor mode
*/
virtual unsigned int  GetScreenY() const=0;

/**
Queues the device to be reset on the beginning of the next frame. This applies any changed adaptor mode or multisample settings.
*/
virtual void  QueueResetDevice()=0;
};

}