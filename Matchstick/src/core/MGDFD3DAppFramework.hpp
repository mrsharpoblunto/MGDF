#pragma once

#include <atomic>
#include "core.impl/MGDFTimer.hpp"
#include "core.impl/MGDFSystemStats.hpp"
#include "MGDFFrameLimiter.hpp"

namespace MGDF
{
namespace core
{

class D3DAppFramework
{
public:
	D3DAppFramework( HINSTANCE hInstance );
	virtual ~D3DAppFramework();

	HINSTANCE GetApplicationInstance();
	HWND GetWindow();
	ID3D11Device *GetD3DDevice() const;

	virtual void OnInitD3D( ID3D11Device *device, IDXGIAdapter1 *adapter ) = 0;
	virtual void OnResetSwapChain( DXGI_SWAP_CHAIN_DESC *, BOOL *fullScreen ) = 0;
	virtual bool IsBackBufferChangePending() = 0;
	virtual void OnBackBufferChanged( ID3D11Texture2D *backBuffer ) = 0;
	virtual void UpdateScene( double elapsedTime ) = 0;
	virtual void DrawScene( double alpha ) = 0;
	virtual void FatalError( const char *sender, const char *message ) = 0;
	virtual void ExternalClose() = 0;
	virtual void OnMouseInput( INT32 x, INT32 y ) = 0;
	virtual void OnRawInput( RAWINPUT *input ) = 0;
	virtual void OnInputIdle() = 0;
	virtual void InitDirect3D( const std::string &caption, WNDPROC windowProcedure, D3D_FEATURE_LEVEL *levels, UINT32 levelsSize );

	LRESULT MsgProc( HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam );

	INT32 Run( UINT32 simulationFps );
protected:

	// Application, Windows, and Direct3D data members.
	ID3D11Device*			_d3dDevice;
	ID3D11DeviceContext*	_immediateContext;

	IDXGISwapChain*			_swapChain;
	IDXGIFactory1*			_factory;

	ID3D11RenderTargetView *_renderTargetView;
	ID3D11DepthStencilView *_depthStencilView;
	ID3D11Texture2D		   *_depthStencilBuffer;
	ID3D11Texture2D		   *_backBuffer;

	DXGI_SWAP_CHAIN_DESC	_swapDesc;
	HINSTANCE				_applicationInstance;
	HWND					_window;

	Timer _timer;
	FrameLimiter *_frameLimiter;
	std::atomic_bool _drawSystemOverlay;
	std::atomic_bool _resize;
	bool _internalShutDown;

	SystemStats _stats;

private:
	void InitMainWindow( const std::string &caption, WNDPROC windowProcedure );
	void InitD3D( D3D_FEATURE_LEVEL *levels, UINT32 levelsSize );
	void InitRawInput();
	void ToggleFullScreenMode();
	void CreateSwapChain();
	void OnResize();

	bool _minimized, _maximized, _resizing;
	boost::thread *_renderThread;
	std::atomic_flag _runRenderThread;
};

//defines a function which calls into an instance of a d3dApp subclass to access the wndproc
#define D3DAPP_WNDPROC(wndProcName,className) LRESULT CALLBACK wndProcName##(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam) { \
	if(##className##!= nullptr ) { return className##->MsgProc(hwnd, msg, wParam, lParam); } \
	else { return DefWindowProc(hwnd, msg, wParam, lParam);}}

}
}
