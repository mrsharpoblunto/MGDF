#pragma once

#include "core.impl/MGDFTimer.hpp"
#include "core.impl/MGDFSystemStats.hpp"
#include "MGDFFrameLimiter.hpp"
#include <boost/thread.hpp>
#include "d3d9.h"

namespace MGDF { namespace core {

class D3DAppFramework
{
public:
	D3DAppFramework(HINSTANCE hInstance);
	virtual ~D3DAppFramework();

	HINSTANCE GetApplicationInstance();
	HWND GetWindow();
	IDirect3DDevice9 *GetD3dDevice();

	virtual void OnInitD3D(DXGI_SWAP_CHAIN_DESC *,IDXGIAdapter1 *adapter)=0;
	virtual void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC *)=0;
	virtual bool IsResetSwapChainPending()=0;
	virtual void OnResize(ID3D11Texture2D *backBuffer)=0;
	virtual void UpdateScene(double elapsedTime) =0;
	virtual void DrawScene(double alpha) =0
	virtual void FatalError(const std::string &errorMessage)=0;
	virtual void ExternalClose()=0;
	virtual void InitDirect3D(const std::string &caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen = true);

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int Run(unsigned int simulationFps);
protected:	

	// Application, Windows, and Direct3D data members.
	ID3D11Device*			_d3dDevice;
	ID3D11DeviceContext*	_immediateContext;
	IDXGISwapChain*			_swapChain;
	IDXGIFactory*			_factory;
	ID3D11RenderTargetView *_renderTargetView;
	ID3D11DepthStencilView *_depthStencilView;
	ID3D11Texture2D		   *_depthStencilBuffer;

	DXGI_SWAP_CHAIN_DESC	_swapDesc;
	HINSTANCE				_applicationInstance;
	HWND					_window;

	Timer _timer;
	FrameLimiter *_frameLimiter;
	unsigned int _width,_height,_canToggleFullScreen,_drawSystemOverlay;
	boost::mutex _renderMutex;
	bool _internalShutDown;
	
	boost::mutex _statsMutex;
	SystemStats _stats;

private:
	void InitMainWindow(const std::string &caption,WNDPROC windowProcedure);
	void InitD3D(D3DDEVTYPE devType, DWORD requestedVP);
	void ToggleFullScreenMode();
	void CreateSwapChain();
	void OnResize();

	bool _minimized;

	bool IsDeviceLost();
	void ResetDevice();
	void DoSimulation();

	boost::thread *_simThread;
	bool _running,_startRendering;
};

//defines a function which calls into an instance of a d3dApp subclass to access the wndproc
#define D3DAPP_WNDPROC(wndProcName,className) LRESULT CALLBACK wndProcName##(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { \
	if(##className##!= NULL ) { return className##->MsgProc(hwnd, msg, wParam, lParam); } \
	else { return DefWindowProc(hwnd, msg, wParam, lParam);}}

}}
