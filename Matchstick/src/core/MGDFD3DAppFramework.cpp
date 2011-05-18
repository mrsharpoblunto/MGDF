#include "StdAfx.h"

#include <math.h>
#include <mmsystem.h>
#include <boost/bind.hpp>
#include "MGDFD3DAppFramework.hpp"
#include "common/MGDFExceptions.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "core.impl/MGDFSystemImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

#define WINDOW_CLASS_NAME "MGDFD3DAppFrameworkWindowClass"
#define STARTING_X 100
#define STARTING_Y 100

D3DAppFramework::D3DAppFramework(HINSTANCE hInstance)
{
	this->_drawSystemOverlay = false;
	this->_applicationInstance = hInstance;
	this->_window = NULL;
	this->_d3dObject = NULL;
	this->_d3dDevice = NULL;
	this->_minimized = false;
	this->_running = false;
	this->_simThread = NULL;
	ZeroMemory(&_d3dPP, sizeof(_d3dPP));
}

D3DAppFramework::~D3DAppFramework()
{
	if (_window!=NULL) {
			UnregisterClass(WINDOW_CLASS_NAME,GetModuleHandle(NULL));
	}
	if (_d3dDevice!=NULL) {
		_d3dDevice->Release();
		_d3dDevice = NULL;
	}
	if (_d3dObject!=NULL) {
		_d3dObject->Release();
		_d3dObject = NULL;
	}
}

HINSTANCE D3DAppFramework::GetApplicationInstance() 
{ 
	return _applicationInstance;
}

HWND D3DAppFramework::GetWindow() 
{ 
	return _window;
}

IDirect3DDevice9 *D3DAppFramework::GetD3dDevice() 
{
	return _d3dDevice;
}

void D3DAppFramework::InitDirect3D(std::string caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen) {
	_canToggleFullScreen = canToggleFullScreen;
	InitMainWindow(caption,windowProcedure);
	InitD3D(devType,requestedVP);
}

void D3DAppFramework::InitMainWindow(std::string caption,WNDPROC windowProcedure)
{
	//if the window has not already been created
	if (_window == NULL) {
		WNDCLASS wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = windowProcedure; 
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = _applicationInstance;
		wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if( !RegisterClass(&wc) )
		{
			FatalError("RegisterClass FAILED");
		}

		RECT R = {0, 0,Resources::MIN_SCREEN_X,Resources::MIN_SCREEN_Y};

		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		_window = CreateWindow(WINDOW_CLASS_NAME, caption.c_str(), 
			WS_OVERLAPPEDWINDOW, STARTING_X, STARTING_Y, R.right, R.bottom, 
			0, 0, _applicationInstance, 0); 

		if( !_window )
		{
			FatalError("CreateWindow FAILED");
		}

		ShowWindow(_window, SW_SHOW);
		UpdateWindow(_window);		
	}
}

void D3DAppFramework::InitD3D(D3DDEVTYPE devType, DWORD requestedVP)
{
	if (_window !=NULL && _d3dObject==NULL) {
		//create Direct3D object
		if((_d3dObject = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		{
			FatalError("Direct3d Direct3DCreate9() failed!");
		}

		//Verify hardware support for specified formats in windowed and full screen modes.	
		D3DDISPLAYMODE mode;
		_d3dObject->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
		
		if (FAILED(_d3dObject->CheckDeviceType(D3DADAPTER_DEFAULT, devType, mode.Format, mode.Format, true))) {
			FatalError("Direct3d checkDeviceType() failed for windowed mode");
		}
		if (FAILED(_d3dObject->CheckDeviceType(D3DADAPTER_DEFAULT, devType, D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, false))) {
			FatalError("Direct3d checkDeviceType() failed for fullscreen mode");
		}

		// Check for requested vertex processing and pure device.
		D3DCAPS9 caps;
		if (FAILED(_d3dObject->GetDeviceCaps(D3DADAPTER_DEFAULT, devType, &caps))) {
			FatalError("Direct3d getDevicecaps() failed");
		}

		DWORD devBehaviorFlags = 0;
		if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) {
			devBehaviorFlags |= requestedVP;
		}
		else {
			devBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}

		// If pure device and HW T&L supported
		if( caps.DevCaps & D3DDEVCAPS_PUREDEVICE && devBehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) {
				devBehaviorFlags |= D3DCREATE_PUREDEVICE;
		}

		OnInitPresentParameters(&_d3dPP,_d3dObject);
		_d3dPP.hDeviceWindow = _window;

		//Create the device.
		if (FAILED(_d3dObject->CreateDevice(
			D3DADAPTER_DEFAULT, // primary adapter
			devType,           // device type
			_window,          // window associated with device
			devBehaviorFlags,   // vertex processing
			&_d3dPP,            // present parameters
			&_d3dDevice))) // return created device
		{ 
			FatalError("Direct3d createDevice() failed");
		}
	}
}

void D3DAppFramework::ResetDevice() {
	OnLostDevice();
	if (FAILED(_d3dDevice->Reset(&_d3dPP))) {
		FatalError("Direct3D Reset() failed");
	}
	OnResetDevice();
}

bool D3DAppFramework::IsDeviceLost()
{
	HRESULT hr = _d3dDevice->TestCooperativeLevel();
	switch(hr)
	{
		case D3DERR_DEVICELOST: 
			return true;
		case D3DERR_DEVICENOTRESET:
			{
				ResetDevice();
				return false;
			}
		case D3DERR_DRIVERINTERNALERROR:
			FatalError("Internal Driver Error");
			return true;
	}
	return false;
}

int D3DAppFramework::Run(unsigned int simulationFps)
{
	//if the window or d3d has not been initialised, quit with an error
	if (_window== NULL || _d3dObject == NULL) {
		return -1;
	}

	try 
	{
		_frameLimiter = new FrameLimiter(simulationFps);
	}
	catch (MGDFException ex)
	{
		FatalError(ex.what());
	}

	//init stats struct
	memset(&_stats,0,sizeof(SystemStats));
	_stats.ExpectedSimTime = 1/(double)simulationFps;

	_startRendering = false;
	_simThread = new boost::thread(boost::bind(&D3DAppFramework::DoSimulation,this));

	while (!_startRendering) Sleep(1);//ensure the simulation runs at least one tick before rendering begins.

	MSG  msg;
    msg.message = WM_NULL;
	while (msg.message != WM_QUIT) {
		//deal with any windows messages
		if(PeekMessage(&msg,0,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			boost::mutex::scoped_lock lock(_renderMutex);
			if(!IsDeviceLost())
			{
				//the game logic step may force the device to reset, so lets check
				if (IsResetDevicePending()) {
					OnResetPresentParameters(&_d3dPP,false);
					ResetDevice();
				}

				if (!_minimized)//don't bother rendering if the window is minimzed
				{
					if(FAILED(_d3dDevice->Clear(0,NULL,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,D3DCOLOR_ARGB(255,0,0,0),1.0f,0))){
						FatalError("Direct3d Clear() failed");
					}

					LARGE_INTEGER renderStart = _timer.GetCurrentTimeTicks();
					LARGE_INTEGER activeRenderEnd;

					// Render the scene
					if( SUCCEEDED(_d3dDevice->BeginScene() ) )
					{
						double alpha = _frameLimiter->ProgressThroughCurrentFrame();
						DrawScene(alpha);//render as per the current active module
						activeRenderEnd = _timer.GetCurrentTimeTicks();

						if(FAILED(_d3dDevice->EndScene())){
							FatalError("Direct3d EndScene() failed");
						}
					}
					else {
						FatalError("Direct3d BeginScene() failed");
					}

					HRESULT hr = _d3dDevice->Present(NULL,NULL,NULL,NULL);
					LARGE_INTEGER renderEnd = _timer.GetCurrentTimeTicks();

					if(hr == D3DERR_DEVICELOST) {
						//do nothing, the device will be restored elsewhere
					}
					else if (FAILED(hr)) {
						FatalError("Direct3d Present() failed");
					}			

					boost::mutex::scoped_lock lock(_statsMutex);
					_stats.RenderTime = _timer.ConvertDifferenceToSeconds(renderEnd,renderStart);
					_stats.ActiveRenderTime = _timer.ConvertDifferenceToSeconds(activeRenderEnd,renderStart);
				}
				
			}
		}
	}

	if (_simThread!=NULL)
	{
		_running = false;
		_simThread->join();
		delete _simThread;
	}

	delete _frameLimiter;
	return (int)msg.wParam;
}

void D3DAppFramework::DoSimulation()
{
	_running = true;
	while (_running)
	{
		LARGE_INTEGER simulationStart = _timer.GetCurrentTimeTicks();

		UpdateScene(_stats.ExpectedSimTime);//run a frame of game logic
		_startRendering = true;

		//wait until the next frame to begin if we have any spare time left over
		_frameLimiter->LimitFps();

		LARGE_INTEGER simulationEnd = _timer.GetCurrentTimeTicks();

		boost::mutex::scoped_lock lock(_statsMutex);
		_stats.SimTime = _timer.ConvertDifferenceToSeconds(simulationEnd,simulationStart);
	}
}

LRESULT D3DAppFramework::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Is the application in a minimized or maximized state?
	static bool minOrMaxed = false;

	RECT clientRect = {0, 0, 0, 0};
	switch( msg )
	{
	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		if(_d3dDevice)
		{
			if (_d3dPP.Windowed) {
				_d3dPP.BackBufferWidth  = LOWORD(lParam) < Resources::MIN_SCREEN_X ? Resources::MIN_SCREEN_X :LOWORD(lParam);
				_d3dPP.BackBufferHeight = HIWORD(lParam)< Resources::MIN_SCREEN_Y ? Resources::MIN_SCREEN_Y :HIWORD(lParam);
			}

			if( wParam == SIZE_MINIMIZED )
			{
				_minimized = true;
				minOrMaxed = true;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				minOrMaxed = true;
				_minimized = false;
				boost::mutex::scoped_lock lock(_renderMutex);
				ResetDevice();
			}
			// Restored is any resize that is not a minimize or maximize.
			// For example, restoring the window to its default size
			// after a minimize or maximize, or from dragging the resize
			// bars.
			else if( wParam == SIZE_RESTORED )
			{
				// Are we restoring from a mimimized or maximized state, 
				if( minOrMaxed && _d3dPP.Windowed)
				{
					_minimized = false;
					boost::mutex::scoped_lock lock(_renderMutex);
					ResetDevice();
				}
				else
				{
					_minimized = false;
					// No, which implies the user is resizing by dragging
					// the resize bars.  However, we do not reset the device
					// here because as the user continuously drags the resize
					// bars, a stream of WM_SIZE messages is sent to the window,
					// and it would be pointless (and slow) to reset for each
					// WM_SIZE message received from dragging the resize bars.
					// So instead, we reset after the user is done resizing the
					// window and releases the resize bars, which sends a
					// WM_EXITSIZEMOVE message.
				}
				minOrMaxed = false;
			}
		}
		return 0;


	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		GetClientRect(_window, &clientRect);
		if (_d3dPP.Windowed) {
			if (clientRect.right<=0) {
				_d3dPP.BackBufferWidth = Resources::MIN_SCREEN_X;
			}
			else {
				_d3dPP.BackBufferWidth  = (unsigned int)clientRect.right < Resources::MIN_SCREEN_X ? Resources::MIN_SCREEN_X :clientRect.right;
			}
			if (clientRect.bottom<=0) {
				_d3dPP.BackBufferHeight = Resources::MIN_SCREEN_Y;
			}
			else {
				_d3dPP.BackBufferHeight  = (unsigned int)clientRect.bottom < Resources::MIN_SCREEN_Y ? Resources::MIN_SCREEN_Y :clientRect.bottom;
			}
		}
		{
			boost::mutex::scoped_lock lock(_renderMutex);
			ResetDevice();
		}
		return 0;

	// WM_CLOSE is sent when the user presses the 'X' button in the
	// caption bar menu.
	case WM_CLOSE:
		DestroyWindow(_window);
		return 0;

	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_SYSKEYDOWN:
		switch( wParam )
		{
		case VK_RETURN:
			{
				// Toggle full screen upon alt-enter 
				DWORD dwMask = (1 << 29);
				// Alt is down and the application can toggle fullscreen mode
				if(_canToggleFullScreen && (lParam & dwMask) != 0 ) {
					ToggleFullScreenMode();
				}
			}
			break;
		case VK_F12:
			{
				//Toggle system stats overlay with alt f12
				_drawSystemOverlay = !_drawSystemOverlay;
			}
			break;
		}
		return 0;
	
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void D3DAppFramework::ToggleFullScreenMode()
{
	//switch to fullscreen
	if(_d3dPP.Windowed ) { 
		//switch to fullscreen present parameters
		OnResetPresentParameters(&_d3dPP,true);

		// Change the window style to a more fullscreen friendly style.
		SetWindowLongPtr(_window, GWL_STYLE, WS_POPUP);

		// If we call SetWindowLongPtr, MSDN states that we need to call
		// SetWindowPos for the change to take effect.  In addition, we 
		// need to call this function anyway to update the window dimensions.
		SetWindowPos(_window, HWND_TOP, 0, 0, _d3dPP.BackBufferWidth , _d3dPP.BackBufferHeight , SWP_NOZORDER | SWP_SHOWWINDOW);	
	}
	// Switch to windowed mode.
	else
	{
		RECT R = {0, 0, Resources::MIN_SCREEN_X,Resources::MIN_SCREEN_Y};
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

		//switch to windowed present parameters
		OnResetPresentParameters(&_d3dPP,true);
		_d3dPP.BackBufferWidth  = Resources::MIN_SCREEN_X;
		_d3dPP.BackBufferHeight = Resources::MIN_SCREEN_Y;

		// Change the window style to a more windowed friendly style.
		SetWindowLongPtr(_window, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		// If we call SetWindowLongPtr, MSDN states that we need to call
		// SetWindowPos for the change to take effect.  In addition, we 
		// need to call this function anyway to update the window dimensions.
		SetWindowPos(_window, HWND_TOP, STARTING_X, STARTING_Y, R.right, R.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
	}

	// Reset the device with the changes.
	boost::mutex::scoped_lock lock(_renderMutex);
	ResetDevice();
}

}}
