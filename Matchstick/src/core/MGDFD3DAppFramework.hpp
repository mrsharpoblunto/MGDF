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

	virtual bool CheckDeviceCaps() =0;
	virtual void OnInitPresentParameters(D3DPRESENT_PARAMETERS *,IDirect3D9* d3d9)=0;
	virtual void OnResetPresentParameters(D3DPRESENT_PARAMETERS *,bool toggleFullScreen)=0;
	virtual void UpdateScene(double elapsedTime) =0;
	virtual void DrawScene(double alpha) =0;
	virtual void OnLostDevice()=0;
	virtual void OnResetDevice()=0;
	virtual bool IsResetDevicePending()=0;
	virtual void FatalError(std::string errorMessage)=0;
	virtual void InitDirect3D(std::string caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen = true);

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int Run(unsigned int simulationFps);
protected:	

	// Application, Windows, and Direct3D data members.
	IDirect3D9*				_d3dObject;
	IDirect3DDevice9*		_d3dDevice;
	D3DPRESENT_PARAMETERS _d3dPP;
	HINSTANCE				_applicationInstance;
	HWND					_window;

	Timer _timer;
	FrameLimiter *_frameLimiter;
	unsigned int _width,_height,_canToggleFullScreen,_drawSystemOverlay;
	boost::mutex _renderMutex;

	boost::mutex _statsMutex;
	SystemStats _stats;

private:
	void InitMainWindow(std::string caption,WNDPROC windowProcedure);
	void InitD3D(D3DDEVTYPE devType, DWORD requestedVP);
	void ToggleFullScreenMode();
	bool _minimized;

	bool IsDeviceLost();
	void ResetDevice();
	void DoSimulation();

	boost::thread *_simThread;
	bool _running;
};

//defines a function which calls into an instance of a d3dApp subclass to access the wndproc
#define D3DAPP_WNDPROC(wndProcName,className) LRESULT CALLBACK wndProcName##(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { \
	if(##className##!= NULL ) { return className##->MsgProc(hwnd, msg, wParam, lParam); } \
	else { return DefWindowProc(hwnd, msg, wParam, lParam);}}

}}
