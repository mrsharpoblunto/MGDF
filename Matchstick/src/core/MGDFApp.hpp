#pragma once

#include <dwrite_1.h>
#include "core.impl/MGDFHostImpl.hpp"
#include "core.impl/MGDFHostStats.hpp"

#include "MGDFD3DAppFramework.hpp"
#include "MGDFFrameLimiter.hpp"

namespace MGDF
{
namespace core
{

class MGDFApp: public D3DAppFramework
{
public:
	MGDFApp( Host *host, HINSTANCE hInstance );
	virtual ~MGDFApp();

protected:
	UINT32 GetCompatibleD3DFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *featureLevelsSize ) override;
	void OnInitDevices( ID3D11Device *d3dDevice, ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter ) override;
	void OnResetSwapChain( DXGI_SWAP_CHAIN_DESC1 &, DXGI_SWAP_CHAIN_FULLSCREEN_DESC&, const RECT& windowSize ) override;
	void OnSwitchToFullScreen( DXGI_MODE_DESC1 & ) override;
	void OnSwitchToWindowed() override;
	void OnResize( UINT32 width, UINT32 height ) override;
	bool IsBackBufferChangePending() override;
	bool VSyncEnabled() const override;
	bool WindowResizingEnabled() const override;
	void OnBeforeBackBufferChange() override;
	void OnBackBufferChange( ID3D11Texture2D *backBuffer ) override;
	void OnBeforeDeviceReset() override;
	void OnBeforeFirstDraw() override;
	void OnDraw() override;
	void OnAfterPresent() override;

	void OnUpdateSim() override;

	LRESULT OnHandleMessage( HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam ) override;
	void OnExternalClose() override;
	void OnRawInput( RAWINPUT *input ) override;
	void OnMouseInput( INT32 x, INT32 y ) override;
	void OnInputIdle() override;

	void FatalError( const char *sender, const char *message ) override;
private:
	void DrawSystemOverlay();
	void InitBrushes();

	bool _initialized;

	Host *_host;
	FrameLimiter * _frameLimiter;

	HostStats _stats;
	LARGE_INTEGER _renderStart;
	LARGE_INTEGER _activeRenderEnd;

	std::atomic_bool _drawSystemOverlay;

	ID2D1DeviceContext *_context;
	ID2D1SolidColorBrush *_whiteBrush;
	ID2D1SolidColorBrush *_blackBrush;
	IDWriteFactory1 *_dWriteFactory;
	IDWriteTextFormat *_textFormat;
};

}
}