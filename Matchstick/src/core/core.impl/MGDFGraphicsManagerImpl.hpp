#pragma once

#include <vector>
#include <boost/thread.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>
#include "d3d9.h"

#include "../common/MGDFListImpl.hpp"

namespace MGDF { namespace core {

class GraphicsAdaptorMode: public IGraphicsAdaptorMode
{
public:
	GraphicsAdaptorMode(unsigned int width,unsigned int height,unsigned int refreshRate)
	{
		_width = width;
		_height = height;
		_refreshRate = refreshRate;
	}

	virtual ~GraphicsAdaptorMode(void){};
	virtual unsigned int GetWidth() const{ return _width; }
	virtual unsigned int GetHeight() const{ return _height; }
	virtual unsigned int GetRefreshRate() const { return _refreshRate; }
private:
	unsigned int _width,_height,_refreshRate;
};

typedef ListImpl<IGraphicsAdaptorModeList,IGraphicsAdaptorMode *> GraphicsAdaptorModeList;


//this class is accessed by the sim and render threads, so setting values and doing device resets must be synced up with a mutex
class GraphicsManager: public IGraphicsManager {
public:
	GraphicsManager(IDirect3D9 *d3d);
	virtual ~GraphicsManager(void);
	virtual bool GetVSync() const;
	virtual void SetVSync(bool vsync);
	virtual unsigned int GetMultiSampleLevels() const;
	virtual void SetCurrentMultiSampleLevel(unsigned int multisampleLevel);
	virtual unsigned int GetCurrentMultiSampleLevel() const;
	virtual const IGraphicsAdaptorModeList *GetAdaptorModes() const;
	virtual IGraphicsAdaptorMode *GetAdaptorMode(unsigned int width,unsigned int height) const;
	virtual IGraphicsAdaptorMode *GetCurrentAdaptorMode() const;
	virtual unsigned int GetScreenX() const;
	virtual unsigned int GetScreenY() const;
	virtual void SetCurrentAdaptorMode(IGraphicsAdaptorMode *mode);
	virtual void QueueResetDevice();

	void LoadPreferences(IGame *game);
	bool IsResetDevicePending();
	void OnResetDevice(D3DPRESENT_PARAMETERS *d3dPP,bool toggleFullScreen);

private:
	IDirect3D9 *_d3d;
	bool _initialized,_resetDevicePending;

	GraphicsAdaptorModeList _adaptorModes;
	DWORD _multiSampleLevelFullScreen;
	DWORD _multiSampleLevelWindowed;

	IGraphicsAdaptorMode *_currentAdaptorMode;
	unsigned int _currentMultiSampleLevel;
	bool _vsync;
	bool _fullScreen;

	boost::mutex _mutex;
};

}}