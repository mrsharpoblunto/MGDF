#include "StdAfx.h"

#include <string.h>
#include <math.h>
#include <mmsystem.h>
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
	: _stats(TIMER_SAMPLES)
	, _drawSystemOverlay(false)
	, _applicationInstance(hInstance)
	, _window(nullptr)
	, _swapChain(nullptr)
	, _factory(nullptr)
	, _immediateContext(nullptr)
	, _d3dDevice(nullptr)
	, _backBuffer(nullptr)
	, _renderTargetView(nullptr)
	, _depthStencilView(nullptr)
	, _depthStencilBuffer(nullptr)
	, _minimized(false)
	, _maximized(false)
	, _resizing(false)
	, _running(false)
	, _simThread(nullptr)
	, _internalShutDown(false)
{
}

D3DAppFramework::~D3DAppFramework()
{
	if (_window!=nullptr) {
		UnregisterClass(WINDOW_CLASS_NAME,GetModuleHandle(nullptr));
	}

	if (_immediateContext)
	{
		_immediateContext->ClearState();
		_immediateContext->Flush();
	}

	if (_swapChain)
	{
		//d3d has to be in windowed mode to cleanup correctly
		_swapChain->SetFullscreenState(false,nullptr);
	}

	SAFE_RELEASE(_backBuffer);
	SAFE_RELEASE(_renderTargetView);
	SAFE_RELEASE(_depthStencilView);
	SAFE_RELEASE(_depthStencilBuffer);
	SAFE_RELEASE(_swapChain);
	SAFE_RELEASE(_factory);
	SAFE_RELEASE(_immediateContext);
	SAFE_RELEASE(_d3dDevice);
}

HINSTANCE D3DAppFramework::GetApplicationInstance() 
{ 
	return _applicationInstance;
}

HWND D3DAppFramework::GetWindow() 
{ 
	return _window;
}

ID3D11Device *D3DAppFramework::GetD3DDevice() const
{
	return _d3dDevice;
}

void D3DAppFramework::InitDirect3D(const std::string &caption,WNDPROC windowProcedure) {
	InitMainWindow(caption,windowProcedure);
	InitD3D();
}

void D3DAppFramework::InitMainWindow(const std::string &caption,WNDPROC windowProcedure)
{
	//if the window has not already been created
	if (_window == nullptr) {
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

void D3DAppFramework::InitD3D()
{
	if (_window != nullptr) {
		UINT createDeviceFlags = 0;
		#if defined(DEBUG) || defined(_DEBUG)  
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif

		if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&_factory)))
		{
			FatalError("Failed to create IDXGIFactory.");
		}

		IDXGIAdapter1 *adapter;
		char videoCardDescription[128];
		DXGI_ADAPTER_DESC1 adapterDesc;
		unsigned int stringLength;

		// step through the adapters until we find a compatible adapter and successfully create a device
		for(int i = 0; _factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
		{
			adapter->GetDesc1(&adapterDesc);
			unsigned int length = wcslen(adapterDesc.Description);
			int error = wcstombs_s(&stringLength, videoCardDescription, 128, adapterDesc.Description, length);		
			std::string message(videoCardDescription,videoCardDescription+length);
			message.insert(0,"Attempting to create device for adapter ");
			GetLoggerImpl()->Add(THIS_NAME,message,LOG_LOW);

			D3D_FEATURE_LEVEL featureLevel;
			if (FAILED(D3D11CreateDevice(
						adapter,
						D3D_DRIVER_TYPE_UNKNOWN,// as we're specifying an adapter to use, we must specify that the driver type is unknown!!!
						0, // no software device
						createDeviceFlags, 
						0, 0,  // default feature level array
						D3D11_SDK_VERSION,
						&_d3dDevice,
						&featureLevel,
						&_immediateContext) || 
				featureLevel != D3D_FEATURE_LEVEL_11_0))
			{
				//if we couldn't create the device, or it doesn't support the dx11 feature set
				SAFE_RELEASE(_immediateContext);
				SAFE_RELEASE(_d3dDevice);
				SAFE_RELEASE(adapter);
			}
			else
			{
				break;
			}
		}

		if( !_d3dDevice )
		{
			FatalError("No adapters found supporting Direct3D Feature Level 11");
		}

		OnInitD3D(_d3dDevice,adapter);
		SAFE_RELEASE(adapter);

		OnResetSwapChain(&_swapDesc,nullptr);
		_swapDesc.OutputWindow = _window;

		CreateSwapChain();
		OnResize();
	}
}

void D3DAppFramework::CreateSwapChain()
{
	if (FAILED(_factory->CreateSwapChain(_d3dDevice, &_swapDesc, &_swapChain)))
	{
		FatalError("Failed to create swap chain");
	}
}

void D3DAppFramework::OnResize()
{
	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
	SAFE_RELEASE(_backBuffer);
	SAFE_RELEASE(_renderTargetView);
	SAFE_RELEASE(_depthStencilView);
	SAFE_RELEASE(_depthStencilBuffer);

	// Resize the swap chain and recreate the render target view.
	if (FAILED(_swapChain->ResizeBuffers(
		1, 
		_swapDesc.BufferDesc.Width, 
		_swapDesc.BufferDesc.Height, 
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)))
	{
		FatalError("Failed to resize swapchain buffers");
	}

	if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&_backBuffer))))
	{
		FatalError("Failed to get swapchain buffer");
	}
	if (FAILED(_d3dDevice->CreateRenderTargetView(_backBuffer, 0, &_renderTargetView)))
	{
		FatalError("Failed to create render target view from backbuffer");
	}

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width     = _swapDesc.BufferDesc.Width;
	depthStencilDesc.Height    = _swapDesc.BufferDesc.Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = _swapDesc.SampleDesc.Count;
	depthStencilDesc.SampleDesc.Quality= _swapDesc.SampleDesc.Quality;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	if (FAILED(_d3dDevice->CreateTexture2D(&depthStencilDesc, 0, &_depthStencilBuffer)))
	{
		FatalError("Failed to create texture from depth stencil description");
	}

	if (FAILED(_d3dDevice->CreateDepthStencilView(_depthStencilBuffer, 0, &_depthStencilView)))
	{
		FatalError("Failed to create depthStencilView from depth stencil buffer");
	}

	// Bind the render target view and depth/stencil view to the pipeline.
	_immediateContext->OMSetRenderTargets(1, &_renderTargetView, _depthStencilView);
	
	// Set the viewport transform.
	D3D11_VIEWPORT viewPort;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width    = static_cast<float>(_swapDesc.BufferDesc.Width);
	viewPort.Height   = static_cast<float>(_swapDesc.BufferDesc.Height);
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;

	_immediateContext->RSSetViewports(1, &viewPort);
	
	OnBackBufferChanged(_backBuffer);
}

int D3DAppFramework::Run(unsigned int simulationFps)
{
	//if the window or d3d has not been initialised, quit with an error
	if (_window == nullptr && _d3dDevice == nullptr) {
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

	//run the simulation on a separate thread to the renderer
	_simThread = new boost::thread([this]()
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
	});
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

			//the game logic step may force the device to reset, so lets check
			if (IsBackBufferChangePending()) 
			{
				//clean up the old swap chain, then recreate it with the new settings
				//also when setting display settings, we'll switch back to fullscreen
				BOOL fullScreen=1;
				SAFE_RELEASE(_swapChain);
				OnResetSwapChain(&_swapDesc,&fullScreen);
				CreateSwapChain();
				OnResize();
			}

			if (!_minimized)//don't bother rendering if the window is minimzed
			{
				float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; //red,green,blue,alpha 
				_immediateContext->ClearRenderTargetView(_renderTargetView, reinterpret_cast<const float*>(&black));
				_immediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

				LARGE_INTEGER renderStart = _timer.GetCurrentTimeTicks();
				LARGE_INTEGER activeRenderEnd;

				// Render the scene
				double alpha = _frameLimiter->ProgressThroughCurrentFrame();
				DrawScene(alpha);//render as per the current active module
				activeRenderEnd = _timer.GetCurrentTimeTicks();

				if (FAILED(_swapChain->Present(_swapDesc.BufferDesc.RefreshRate.Numerator!=1U,0)))
				{
					FatalError("Direct3d Present() failed");
				}
				LARGE_INTEGER renderEnd = _timer.GetCurrentTimeTicks();		

				boost::mutex::scoped_lock lock(_statsMutex);
				_stats.AppendRenderTime(_timer.ConvertDifferenceToSeconds(renderEnd,renderStart));
				_stats.AppendActiveRenderTime(_timer.ConvertDifferenceToSeconds(activeRenderEnd,renderStart));
			}
		}
	}

	if (_simThread!=nullptr)
	{
		_running = false;
		_simThread->join();
		delete _simThread;
	}

	delete _frameLimiter;
	return (int)msg.wParam;
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
		//TODO need member variable for size, so setting full screen doesn't overwrite it,
		_swapDesc.BufferDesc.Width  = LOWORD(lParam) < Resources::MIN_SCREEN_X ? Resources::MIN_SCREEN_X :LOWORD(lParam);
		_swapDesc.BufferDesc.Height = HIWORD(lParam)< Resources::MIN_SCREEN_Y ? Resources::MIN_SCREEN_Y :HIWORD(lParam);

		if(_d3dDevice)
		{
			if( wParam == SIZE_MINIMIZED )
			{
				_minimized = true;
				_maximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				_maximized = true;
				_minimized = false;
				boost::mutex::scoped_lock lock(_renderMutex);
				OnResize();
			}
			// Restored is any resize that is not a minimize or maximize.
			// For example, restoring the window to its default size
			// after a minimize or maximize, or from dragging the resize
			// bars.
			else if( wParam == SIZE_RESTORED )
			{
				// Are we restoring from a mimimized or maximized state, 
				if(_minimized)
				{
					_minimized = false;
					boost::mutex::scoped_lock lock(_renderMutex);
					OnResize();
				}
				else if(_maximized)
				{
					_maximized = false;
					boost::mutex::scoped_lock lock(_renderMutex);
					OnResize();
				}
				else if (_resizing)
				{
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
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		_resizing  = true;
		return 0;

	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		_resizing = false;
		{
			boost::mutex::scoped_lock lock(_renderMutex);
			OnResize();
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

}}
