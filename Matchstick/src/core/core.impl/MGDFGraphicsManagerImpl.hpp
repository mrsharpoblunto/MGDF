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
	GraphicsAdaptorMode(
		unsigned int width,
		unsigned int height,
		unsigned int refreshRateNumerator,
		unsigned int refreshRateDenominator)
	{
		_width = width;
		_height = height;
		_refreshRateNumerator = refreshRateNumerator;
		_refreshRateDenominator = refreshRateDenominator;
	}

	virtual ~GraphicsAdaptorMode(void){};
	virtual unsigned int GetWidth() const{ return _width; }
	virtual unsigned int GetHeight() const{ return _height; }
	virtual unsigned int GetRefreshRateNumerator() const { return _refreshRateNumerator; }
	virtual unsigned int GetRefreshRateDenominator() const { return _refreshRateDenominator; }
private:
	unsigned int _width,_height,_refreshRateNumerator,_refreshRateDenominator;
};

typedef ListImpl<IGraphicsAdaptorModeList,IGraphicsAdaptorMode *> GraphicsAdaptorModeList;
typedef ListImpl<IUIntList,unsigned int> UIntList;

//this class is accessed by the sim and render threads, so setting values and doing device resets must be synced up with a mutex
class GraphicsManager: public IGraphicsManager {
public:
	GraphicsManager(ID3D11Device *device,IDXGIAdapter1 *adapter);
	virtual ~GraphicsManager(void);
	virtual bool GetVSync() const;
	virtual void SetVSync(bool vsync);
	virtual IUIntList *GetMultiSampleLevels() const;
	virtual bool SetBackBufferMultiSampleLevel(unsigned int multisampleLevel);
	virtual unsigned int GetBackBufferMultiSampleLevel() const;
	virtual bool SetCurrentMultiSampleLevel(unsigned int multisampleLevel);
	virtual unsigned int GetCurrentMultiSampleLevel(unsigned int *quality) const;
	virtual const IGraphicsAdaptorModeList *GetAdaptorModes() const;
	virtual IGraphicsAdaptorMode *GetAdaptorMode(unsigned int width,unsigned int height) const;
	virtual IGraphicsAdaptorMode *GetCurrentAdaptorMode() const;
	virtual unsigned int GetScreenX() const;
	virtual unsigned int GetScreenY() const;
	virtual void SetCurrentAdaptorMode(IGraphicsAdaptorMode *mode);
	virtual void ApplyChanges();
	virtual ID3D11Texture2D *GetBackBuffer() const;
	virtual void GetBackBufferDescription(D3D11_TEXTURE2D_DESC *desc) const;
	virtual ID3D11Device *GetD3DDevice() const;

	void LoadPreferences(IGame *game);
	bool IsBackBufferChangePending();
	void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *desc,BOOL *fullScreen);
	void SetBackBuffer(ID3D11Texture2D *backBuffer);
private:
	bool _initialized,_changePending;
	ID3D11Device *_device;
	ID3D11Texture2D *_backBuffer;

	GraphicsAdaptorModeList _adaptorModes;
	IGraphicsAdaptorMode *_currentAdaptorMode;

	UIntList _multiSampleLevels;
	std::map<unsigned int,unsigned int> _multiSampleQuality;
	unsigned int _currentMultiSampleLevel;
	unsigned int _backBufferMultiSampleLevel;

	bool _vsync;
	bool _fullScreen;

	boost::mutex _mutex;
};

}}