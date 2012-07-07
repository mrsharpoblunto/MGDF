#pragma once

#include <vector>
#include <boost/thread.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>
#include "d3d11.h"

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
typedef ListImpl<IUIntList,unsigned int> UIntList;

//this class is accessed by the sim and render threads, so setting values and doing device resets must be synced up with a mutex
class GraphicsManager: public IGraphicsManager {
public:
	GraphicsManager(IDXGIAdapter1 *adapter,ID3D11Device *device);
	virtual ~GraphicsManager(void);
	virtual bool GetVSync() const;
	virtual void SetVSync(bool vsync);
	virtual IUIntList *GetMultiSampleLevels() const;
	virtual bool SetCurrentMultiSampleLevel(unsigned int multisampleLevel);
	virtual unsigned int GetCurrentMultiSampleLevel() const;
	virtual const IGraphicsAdaptorModeList *GetAdaptorModes() const;
	virtual IGraphicsAdaptorMode *GetAdaptorMode(unsigned int width,unsigned int height) const;
	virtual IGraphicsAdaptorMode *GetCurrentAdaptorMode() const;
	virtual unsigned int GetScreenX() const;
	virtual unsigned int GetScreenY() const;
	virtual void SetCurrentAdaptorMode(IGraphicsAdaptorMode *mode);
	virtual void QueueResetSwapChain();

	void LoadPreferences(IGame *game);
	bool IsResetPending();
	void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *desc);

private:
	bool _initialized,_resetPending;
	ID3D11dEVICE *_device;

	GraphicsAdaptorModeList _adaptorModes;
	IGraphicsAdaptorMode *_currentAdaptorMode;

	UIntList _multiSampleLevels;
	std::map<unsigned int,unsigned int> _multiSampleQuality;
	unsigned int _currentMultiSampleLevel;

	bool _vsync;
	bool _fullScreen;

	boost::mutex _mutex;
};

}}