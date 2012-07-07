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

	D3DAppFramework::D3DAppFramework(HINSTANCE hInstance): _stats(TIMER_SAMPLES)
{
	this->_drawSystemOverlay = false;
	this->_applicationInstance = hInstance;
	this->_window = NULL;
	this->_d3dObject = NULL;
	this->_d3dDevice = NULL;
	this->_minimized = false;
	this->_running = false;
	this->_simThread = NULL;
	this->_internalShutDown = false;
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

void D3DAppFramework::InitDirect3D(const std::string &caption,WNDPROC windowProcedure,D3DDEVTYPE devType, DWORD requestedVP,bool canToggleFullScreen) {
	_canToggleFullScreen = canToggleFullScreen;
	InitMainWindow(caption,windowProcedure);
	InitD3D(devType,requestedVP);
}

void D3DAppFramework::InitMainWindow(const std::string &caption,WNDPROC windowProcedure)
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
	if (_window !=NULL) {
		UINT createDeviceFlags = 0;
		#if defined(DEBUG) || defined(_DEBUG)  
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif

		IDXGIFactory1* factory = NULL;

		if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory1), (void**)&_factory)))
		{
			FatalError("Failed to create IDXGIFactory.");
		}

		IDXGIAdapter1 *adapter;
		char videoCardDescription[128];
		DXGI_ADAPTER_DESC1 adapterDesc;
		unsigned int stringLength;

		// step through the adapters until we find a compatible adapter and successfully create a device
		for(int i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
		{
			adapter->GetDesc1(&adapterDesc);
			int error = wcstombs_s(&stringLength, videoCardDescription, 128, adapterDesc.Description, 128);		
			std::string message(videoCardDescription,videoCardDescription+128);
			message.insert(0,"Attempting to create device for adapter ");
			message.append("...");
			GetLoggerImpl()->Add("MGDF",message,LOG_LOW);

			driverType = D3D_DRIVER_TYPE_UNKNOWN;// as we're specifying an adapter to use, we must specify that the driver type is unknown!!!

			D3D_FEATURE_LEVEL featureLevel;
			if (FAILED(D3D11CreateDevice(
						adapter,
						driverType,
						0, // no software device
						createDeviceFlags, 
						0, 0,  // default feature level array
						D3D11_SDK_VERSION,
						&_d3dDevice,
						&featureLevel,
						&_immediateContext) || 
				featureLevel != D3D_FEATURE_LEVEL_11_0)
			{
				//if we couldn't create the device, or it doesn't support the dx11 feature set
				SAFE_RELEASE(_d3dDevice);
				SAFE_RELEASE(_immediateContext);
			}
		}

		if( !_d3dDevice )
		{
			FatalError("No adapters found supporting Direct3D Feature Level 11.");
		}

		_d3dDevice->GetImmediateContext(&_immediateContext);

		OnInitD3D(device,adapter);
		OnResetSwapChain(&_swapDesc);
		_swapDesc.OutputWindow = _window;

		CreateSwapChain();
		OnResize();
	}
}

void D3DAppFramework::CreateSwapChain()
{
	IDXGIDevice* dxgiDevice = 0;
	HR(_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
	      
	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(_d3dDevice, &_swapDesc, &_swapChain));
	
	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);
}

void D3DAppFramework::OnResize()
{
	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
	ReleaseCOM(_renderTargetView);
	ReleaseCOM(_depthStencilView);
	ReleaseCOM(_depthStencilBuffer);


	// Resize the swap chain and recreate the render target view.

	if (FAILED(_swapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0)))
	{
	}

	ID3D11Texture2D* backBuffer;
	HR(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(md3dDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView));
	ReleaseCOM(backBuffer);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	
	depthStencilDesc.Width     = mClientWidth;
	depthStencilDesc.Height    = mClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if( mEnable4xMsaa )
	{
		depthStencilDesc.SampleDesc.Count   = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count   = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	HR(md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer));
	HR(md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView));


	// Bind the render target view and depth/stencil view to the pipeline.

	md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	

	// Set the viewport transform.

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);
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

	_stats.SetExpectedSimTime(1/(double)simulationFps);

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
				if (IsResetSwapChainPending()) {
					OnResetSwapChain(&_swapDesc);
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

						if(FAILED(_d3dDevice->EndScene())){
							FatalError("Direct3d EndScene() failed");
						}
						activeRenderEnd = _timer.GetCurrentTimeTicks();
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
					_stats.AppendRenderTime(_timer.ConvertDifferenceToSeconds(renderEnd,renderStart));
					_stats.AppendActiveRenderTime(_timer.ConvertDifferenceToSeconds(activeRenderEnd,renderStart));
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

		UpdateScene(_stats.ExpectedSimTime());//run a frame of game logic
		_startRendering = true;

		//wait until the next frame to begin if we have any spare time left over
		_frameLimiter->LimitFps();

		LARGE_INTEGER simulationEnd = _timer.GetCurrentTimeTicks();

		boost::mutex::scoped_lock lock(_statsMutex);
		_stats.AppendSimTime(_timer.ConvertDifferenceToSeconds(simulationEnd,simulationStart));
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
		if (_internalShutDown)
		{
			//if we triggered this, then shut down
			DestroyWindow(_window);
		}
		else
		{
			//otherwise just inform the rest of the system that
			//it should shut down ASAP, but give it time to shut down cleanly
			ExternalClose();
		}
		return 0;

	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

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
