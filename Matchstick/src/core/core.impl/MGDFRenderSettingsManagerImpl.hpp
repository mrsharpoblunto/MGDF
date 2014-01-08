#pragma once

#include <vector>
#include <boost/thread.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFRenderSettingsManager.hpp>
#include <d3d11.h>
#include <d3d11_1.h>
#include <atomic>

#include "../common/MGDFListImpl.hpp"

namespace MGDF
{
namespace core
{

//this class is accessed by the sim and render threads, so setting values and doing device resets must be synced up with a mutex
class RenderSettingsManager: public IRenderSettingsManager
{
public:
	RenderSettingsManager();
	virtual ~RenderSettingsManager();

	bool GetVSync() const override;
	void SetVSync( bool vsync ) override;

	bool GetFullscreen() const override;
	void SetFullscreen( bool fullscreen ) override;

	UINT32 GetMultiSampleLevelCount() const override;
	bool GetMultiSampleLevel( UINT32 index, UINT32* level ) const override;

	bool SetBackBufferMultiSampleLevel( UINT32 multisampleLevel ) override;
	UINT32 GetBackBufferMultiSampleLevel() const override;
	bool SetCurrentMultiSampleLevel( UINT32 multisampleLevel ) override;
	UINT32 GetCurrentMultiSampleLevel( UINT32 *quality ) const override;

	UINT32 GetAdaptorModeCount() const override;
	bool GetAdaptorMode( UINT32 index, AdaptorMode *mode ) const override;
	bool GetAdaptorMode( UINT32 width, UINT32 height, AdaptorMode *mode ) const override;
	void GetCurrentAdaptorMode( AdaptorMode *mode ) const override;
	bool SetCurrentAdaptorMode( const AdaptorMode *mode ) override;

	UINT32 GetScreenX() const override;
	UINT32 GetScreenY() const override;
	void ApplyChanges() override;

	void LoadPreferences( IGame *game );

	void InitFromDevice(  ID3D11Device *d3dDevice, IDXGIAdapter1 *adapter );
	bool IsBackBufferChangePending();
	void OnResetSwapChain( DXGI_SWAP_CHAIN_DESC1 &desc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC& fullscreenDesc, const RECT& windowSize );
	void OnSwitchToFullScreen( DXGI_MODE_DESC1 &desc );
	void OnResize( UINT32 width, UINT32 height );
private:
	void Cleanup();

	std::atomic_bool _changePending;

	std::vector<AdaptorMode> _adaptorModes;
	AdaptorMode _currentAdaptorMode;

	std::vector<UINT32> _multiSampleLevels;
	std::map<UINT32, UINT32> _multiSampleQuality;
	UINT32 _currentMultiSampleLevel;
	UINT32 _backBufferMultiSampleLevel;

	UINT32 _screenX, _screenY;

	bool _vsync;
	bool _fullScreen;

	mutable boost::mutex _mutex;
};

}
}