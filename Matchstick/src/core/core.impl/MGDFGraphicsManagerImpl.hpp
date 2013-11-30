#pragma once

#include <vector>
#include <boost/thread.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>
#include <d3d11.h>
#include <atomic>

#include "../common/MGDFListImpl.hpp"

namespace MGDF
{
namespace core
{

class GraphicsAdaptorMode: public IGraphicsAdaptorMode
{
public:
	GraphicsAdaptorMode(
	    UINT32 width,
	    UINT32 height,
	    UINT32 refreshRateNumerator,
	    UINT32 refreshRateDenominator )
		: _width( width )
		, _height( height )
		, _refreshRateNumerator( refreshRateNumerator )
		, _refreshRateDenominator( refreshRateDenominator ) {
	}

	virtual ~GraphicsAdaptorMode( void ) {}
	virtual UINT32 GetWidth() const {
		return _width;
	}
	virtual UINT32 GetHeight() const {
		return _height;
	}
	virtual UINT32 GetRefreshRateNumerator() const {
		return _refreshRateNumerator;
	}
	virtual UINT32 GetRefreshRateDenominator() const {
		return _refreshRateDenominator;
	}
private:
	UINT32 _width, _height, _refreshRateNumerator, _refreshRateDenominator;
};

typedef ListImpl<IGraphicsAdaptorModeList, IGraphicsAdaptorMode *> GraphicsAdaptorModeList;
typedef ListImpl<IUIntList, UINT32> UIntList;

//this class is accessed by the sim and render threads, so setting values and doing device resets must be synced up with a mutex
class GraphicsManager: public IGraphicsManager
{
public:
	GraphicsManager( ID3D11Device *d3dDevice, ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter );
	virtual ~GraphicsManager();
	virtual bool GetVSync() const;
	virtual void SetVSync( bool vsync );
	virtual IUIntList *GetMultiSampleLevels() const;
	virtual bool SetBackBufferMultiSampleLevel( UINT32 multisampleLevel );
	virtual UINT32 GetBackBufferMultiSampleLevel() const;
	virtual bool SetCurrentMultiSampleLevel( UINT32 multisampleLevel );
	virtual UINT32 GetCurrentMultiSampleLevel( UINT32 *quality ) const;
	virtual const IGraphicsAdaptorModeList *GetAdaptorModes() const;
	virtual IGraphicsAdaptorMode *GetAdaptorMode( UINT32 width, UINT32 height ) const;
	virtual IGraphicsAdaptorMode *GetCurrentAdaptorMode() const;
	virtual UINT32 GetScreenX() const;
	virtual UINT32 GetScreenY() const;
	virtual void SetCurrentAdaptorMode( IGraphicsAdaptorMode *mode );
	virtual void ApplyChanges();
	virtual ID3D11Texture2D *GetBackBuffer() const;
	virtual bool SetBackBufferRenderTarget(ID2D1DeviceContext *context);
	virtual void GetBackBufferDescription( D3D11_TEXTURE2D_DESC *desc ) const;
	virtual ID3D11Device *GetD3DDevice() const;
	virtual ID2D1Device *GetD2DDevice() const;

	void LoadPreferences( IGame *game );
	bool IsBackBufferChangePending();
	void OnResetSwapChain( DXGI_SWAP_CHAIN_DESC *desc, BOOL *fullScreen );
	void SetBackBuffer( ID3D11Texture2D *backBuffer );
private:
	bool _initialized;
	std::atomic_bool _changePending;
	ID3D11Device *_d3dDevice;
	ID3D11Texture2D *_backBuffer;

	ID2D1Device *_d2dDevice;

	GraphicsAdaptorModeList _adaptorModes;
	IGraphicsAdaptorMode *_currentAdaptorMode;

	UIntList _multiSampleLevels;
	std::map<UINT32, UINT32> _multiSampleQuality;
	UINT32 _currentMultiSampleLevel;
	UINT32 _backBufferMultiSampleLevel;

	bool _vsync;
	bool _fullScreen;

	boost::mutex _mutex;
};

}
}