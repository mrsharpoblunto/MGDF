#include "StdAfx.h"

#include <string.h>
#include <math.h>
#include <mmsystem.h>
#include "windowsx.h"
#include "MGDFD3DAppFramework.hpp"
#include "common/MGDFExceptions.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "core.impl/MGDFSystemImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

#define WINDOW_CLASS_NAME "MGDFD3DAppFrameworkWindowClass"

D3DAppFramework::D3DAppFramework( HINSTANCE hInstance )
	: _stats( TIMER_SAMPLES )
	, _applicationInstance( hInstance )
	, _window( nullptr )
	, _swapChain( nullptr )
	, _factory( nullptr )
	, _immediateContext( nullptr )
	, _d3dDevice( nullptr )
	, _backBuffer( nullptr )
	, _renderTargetView( nullptr )
	, _depthStencilView( nullptr )
	, _depthStencilBuffer( nullptr )
	, _minimized( false )
	, _maximized( false )
	, _resizing( false )
	, _renderThread( nullptr )
	, _internalShutDown( false )
{
	_drawSystemOverlay.store( false );
	_resize.store( false );
}

D3DAppFramework::~D3DAppFramework()
{
	if ( _window != nullptr ) {
		UnregisterClass( WINDOW_CLASS_NAME, GetModuleHandle( nullptr ) );
	}

	if ( _immediateContext ) {
		_immediateContext->ClearState();
		_immediateContext->Flush();
	}

	if ( _swapChain ) {
		//d3d has to be in windowed mode to cleanup correctly
		_swapChain->SetFullscreenState( false, nullptr );
	}

	SAFE_RELEASE( _backBuffer );
	SAFE_RELEASE( _renderTargetView );
	SAFE_RELEASE( _depthStencilView );
	SAFE_RELEASE( _depthStencilBuffer );
	SAFE_RELEASE( _swapChain );
	SAFE_RELEASE( _factory );
	SAFE_RELEASE( _immediateContext );
	SAFE_RELEASE( _d3dDevice );
	SAFE_RELEASE( _d2dDevice );
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

void D3DAppFramework::InitDirect3D( const std::string &caption, WNDPROC windowProcedure, D3D_FEATURE_LEVEL *levels, UINT32 levelsSize )
{
	InitMainWindow( caption, windowProcedure );
	InitRawInput();
	InitD3D( levels, levelsSize );
}

void D3DAppFramework::InitMainWindow( const std::string &caption, WNDPROC windowProcedure )
{
	//if the window has not already been created
	if ( _window == nullptr ) {
		WNDCLASS wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = windowProcedure;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = _applicationInstance;
		wc.hIcon         = LoadIcon( 0, IDI_APPLICATION );
		wc.hCursor       = LoadCursor( 0, IDC_ARROW );
		wc.hbrBackground = ( HBRUSH ) GetStockObject( WHITE_BRUSH );
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if ( !RegisterClass( &wc ) ) {
			FATALERROR( this, "RegisterClass FAILED" );
		}

		RECT R = {0, 0, Resources::MIN_SCREEN_X, Resources::MIN_SCREEN_Y};
		AdjustWindowRect( &R, WS_OVERLAPPEDWINDOW, false );
		INT32 width  = R.right - R.left;
		INT32 height = R.bottom - R.top;

		_window = CreateWindow( WINDOW_CLASS_NAME, caption.c_str(),
		                        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, _applicationInstance, 0 );

		if ( !_window ) {
			FATALERROR( this, "CreateWindow FAILED" );
		}

		ShowWindow( _window, SW_SHOW );
		UpdateWindow( _window );
	}
}

void D3DAppFramework::InitRawInput()
{
	RAWINPUTDEVICE Rid[2] ;

	Rid[0].usUsagePage = 0x01 ;  // desktop input
	Rid[0].usUsage = 0x02 ;      // mouse
	Rid[0].dwFlags =  0;
	Rid[0].hwndTarget = _window;

	Rid[1].usUsagePage = 0x01 ;  // desktop input
	Rid[1].usUsage = 0x06 ;      // keyboard
	Rid[1].dwFlags = RIDEV_NOHOTKEYS; // disable windows key and other windows hotkeys while the game has focus
	Rid[1].hwndTarget = _window ;

	if ( !RegisterRawInputDevices( Rid, 2, sizeof( Rid[0] ) ) ) {
		FATALERROR( this, "Failed to register raw input devices for mouse and keyboard" );
	}
}

void D3DAppFramework::InitD3D( D3D_FEATURE_LEVEL *levels, UINT32 levelsSize )
{
	if ( _window != nullptr ) {
		UINT32 createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		if ( FAILED( CreateDXGIFactory1( __uuidof( IDXGIFactory1 ), ( void** ) &_factory ) ) ) {
			FATALERROR( this, "Failed to create IDXGIFactory." );
		}

		IDXGIAdapter1 *adapter = nullptr;
		IDXGIAdapter1 *bestAdapter = nullptr;
		char videoCardDescription[128];
		DXGI_ADAPTER_DESC1 adapterDesc;
		size_t stringLength;

		// step through the adapters and ensure we use the best one to create our device
		for ( INT32 i = 0; _factory->EnumAdapters1( i, &adapter ) != DXGI_ERROR_NOT_FOUND; i++ ) {
			adapter->GetDesc1( &adapterDesc );
			size_t length = wcslen( adapterDesc.Description );
			INT32 error = wcstombs_s( &stringLength, videoCardDescription, 128, adapterDesc.Description, length );
			std::string message( videoCardDescription, videoCardDescription + length );
			message.insert( 0, "Attempting to create device for adapter " );
			LOG( message, LOG_LOW );

			D3D_FEATURE_LEVEL featureLevel;
			ID3D11Device *device = nullptr;
			ID3D11DeviceContext *context = nullptr;

			if ( FAILED( D3D11CreateDevice(
			                 adapter,
			                 D3D_DRIVER_TYPE_UNKNOWN,// as we're specifying an adapter to use, we must specify that the driver type is unknown!!!
			                 0, // no software device
			                 createDeviceFlags,
			                 levels, levelsSize,  // default feature level array
			                 D3D11_SDK_VERSION,
			                 &device,
			                 &featureLevel,
			                 &context ) ) ||
			        featureLevel == 0 ) {
				//if we couldn't create the device, or it doesn't support one of our specified feature sets
				SAFE_RELEASE( context );
				SAFE_RELEASE( device );
				SAFE_RELEASE( adapter );
			} else {
				//this is the first acceptable adapter, or the best one so far
				if ( _d3dDevice == nullptr || featureLevel > _d3dDevice->GetFeatureLevel() ) {
					//clear out the previous best adapter
					SAFE_RELEASE( _immediateContext );
					SAFE_RELEASE( _d3dDevice );
					SAFE_RELEASE( bestAdapter );

					//store the new best adapter
					bestAdapter = adapter;
					_d3dDevice = device;
					_immediateContext = context;
					LOG( "Adapter is the best found so far", LOG_LOW );
				}
				//this adapter is no better than what we already have, so ignore it
				else {
					SAFE_RELEASE( context );
					SAFE_RELEASE( device );
					SAFE_RELEASE( adapter );
					LOG( "A better adapter has already been found - Ignoring", LOG_LOW );
				}
			}
		}

		if ( !_d3dDevice ) {
			FATALERROR( this, "No adapters found supporting The specified D3D Feature set" );
		} else {
			LOG( "Created device with D3D Feature level: " << _d3dDevice->GetFeatureLevel(), LOG_LOW );
		}

		D2D1_FACTORY_OPTIONS options;
#if defined(DEBUG) || defined(_DEBUG)
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
		options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

		ID2D1Factory1 *d2dFactory;
		if ( FAILED( D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options,&d2dFactory ) ) ) {
			FATALERROR( this, "Unable to create ID2DFactory1" );
		}

		IDXGIDevice *dxgiDevice;
		if ( FAILED(_d3dDevice->QueryInterface<IDXGIDevice>(&dxgiDevice))) {
			FATALERROR( this, "Unable to acquire IDXGIDevice from ID3D11Device" );
		}

		HRESULT result = d2dFactory->CreateDevice( dxgiDevice, &_d2dDevice );

		if ( FAILED( result )) {
			FATALERROR( this, "Unable to create ID2DDevice1" );
		}

		OnInit( _d3dDevice, _d2dDevice, bestAdapter );
		SAFE_RELEASE( bestAdapter );

		SAFE_RELEASE( dxgiDevice );
		SAFE_RELEASE( d2dFactory );

		OnResetSwapChain( &_swapDesc, nullptr );
		_swapDesc.OutputWindow = _window;

		CreateSwapChain();
		OnResize();
	}
}

void D3DAppFramework::CreateSwapChain()
{
	if ( FAILED( _factory->CreateSwapChain( _d3dDevice, &_swapDesc, &_swapChain ) ) ) {
		FATALERROR( this, "Failed to create swap chain" );
	}
}

void D3DAppFramework::OnResize()
{
	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
	SAFE_RELEASE( _backBuffer );
	SAFE_RELEASE( _renderTargetView );
	SAFE_RELEASE( _depthStencilView );
	SAFE_RELEASE( _depthStencilBuffer );

	// Resize the swap chain and recreate the render target view.
	if ( FAILED( _swapChain->ResizeBuffers(
	                 1,
	                 _swapDesc.BufferDesc.Width,
	                 _swapDesc.BufferDesc.Height,
	                 DXGI_FORMAT_R8G8B8A8_UNORM,
	                 DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) ) ) {
		FATALERROR( this, "Failed to resize swapchain buffers" );
	}

	if ( FAILED( _swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &_backBuffer ) ) ) ) {
		FATALERROR( this, "Failed to get swapchain buffer" );
	}
	if ( FAILED( _d3dDevice->CreateRenderTargetView( _backBuffer, 0, &_renderTargetView ) ) ) {
		FATALERROR( this, "Failed to create render target view from backbuffer" );
	}

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width     = _swapDesc.BufferDesc.Width;
	depthStencilDesc.Height    = _swapDesc.BufferDesc.Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = _swapDesc.SampleDesc.Count;
	depthStencilDesc.SampleDesc.Quality = _swapDesc.SampleDesc.Quality;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags      = 0;

	if ( FAILED( _d3dDevice->CreateTexture2D( &depthStencilDesc, 0, &_depthStencilBuffer ) ) ) {
		FATALERROR( this, "Failed to create texture from depth stencil description" );
	}

	if ( FAILED( _d3dDevice->CreateDepthStencilView( _depthStencilBuffer, 0, &_depthStencilView ) ) ) {
		FATALERROR( this, "Failed to create depthStencilView from depth stencil buffer" );
	}

	// Bind the render target view and depth/stencil view to the pipeline.
	_immediateContext->OMSetRenderTargets( 1, &_renderTargetView, _depthStencilView );

	// Set the viewport transform.
	D3D11_VIEWPORT viewPort;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width    = static_cast<float>( _swapDesc.BufferDesc.Width );
	viewPort.Height   = static_cast<float>( _swapDesc.BufferDesc.Height );
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;

	_immediateContext->RSSetViewports( 1, &viewPort );

	OnBackBufferChanged( _backBuffer );
}

INT32 D3DAppFramework::Run( UINT32 simulationFps )
{
	//if the window or d3d has not been initialised, quit with an error
	if ( _window == nullptr && _d3dDevice == nullptr ) {
		return -1;
	}

	try {
		_frameLimiter = new FrameLimiter( simulationFps );
	} catch ( MGDFException ex ) {
		FATALERROR( this, ex.what() );
	}

	_stats.SetExpectedSimTime( 1 / ( double ) simulationFps );

	std::atomic_flag runSimThread;
	std::atomic_flag waitOnRenderThread;
	waitOnRenderThread.test_and_set();
	_runRenderThread.test_and_set();

	//run the simulation in its own thread
	boost::thread simThread( [this, &runSimThread, &waitOnRenderThread]() {
		runSimThread.test_and_set();

		while ( runSimThread.test_and_set() ) {
			LARGE_INTEGER simulationStart = _timer.GetCurrentTimeTicks();

			UpdateScene( _stats.ExpectedSimTime() );

			//run a frame of game logic before starting the render thread
			waitOnRenderThread.clear();

			//wait until the next frame to begin if we have any spare time left over
			_frameLimiter->LimitFps();

			LARGE_INTEGER simulationEnd = _timer.GetCurrentTimeTicks();
			_stats.AppendSimTime( _timer.ConvertDifferenceToSeconds( simulationEnd, simulationStart ) );
		}
	} );

	//run the renderer in its own thread
	_renderThread = new boost::thread( [this, &waitOnRenderThread]() {
		while ( waitOnRenderThread.test_and_set() ) Sleep( 1 );   //ensure the simulation runs at least one tick before rendering begins.

		while ( _runRenderThread.test_and_set() ) {
			bool exp=true;

			//the game logic step may force the device to reset, so lets check
			if ( IsBackBufferChangePending() ) {
				//clean up the old swap chain, then recreate it with the new settings
				//also when setting display settings, we'll switch back to fullscreen
				BOOL fullScreen = 1;
				SAFE_RELEASE( _swapChain );
				OnResetSwapChain( &_swapDesc, &fullScreen );
				CreateSwapChain();
				OnResize();
			}
			//a window event may also have triggered a resize event.
			else if ( _resize.compare_exchange_strong( exp, false ) ) {
				OnResize();
			}

			if ( !_minimized ) { //don't bother rendering if the window is minimzed
				const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; //RGBA
				_immediateContext->ClearRenderTargetView( _renderTargetView, reinterpret_cast<const float*>( &black ) );
				_immediateContext->ClearDepthStencilView( _depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

				LARGE_INTEGER renderStart = _timer.GetCurrentTimeTicks();
				LARGE_INTEGER activeRenderEnd;

				// Render the scene
				double alpha = _frameLimiter->ProgressThroughCurrentFrame();
				DrawScene( alpha );  //render as per the current active module
				activeRenderEnd = _timer.GetCurrentTimeTicks();

				if ( FAILED( _swapChain->Present( _swapDesc.BufferDesc.RefreshRate.Numerator != 1U, 0 ) ) ) {
					FATALERROR( this, "Direct3d Present() failed" );
				}

				LARGE_INTEGER renderEnd = _timer.GetCurrentTimeTicks();
				_stats.AppendRenderTimes(
				    _timer.ConvertDifferenceToSeconds( renderEnd, renderStart ),
				    _timer.ConvertDifferenceToSeconds( activeRenderEnd, renderStart ) );
			}
		}
	} );

	MSG  msg;
	msg.message = WM_NULL;
	while ( msg.message != WM_QUIT ) {
		//deal with any windows messages on the main thread, this allows us
		//to ensure that any user input is handled with as little latency as possible
		//independant of the update rate for the sim and render threads.
		if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		} else {
			OnInputIdle();
			//don't hog the CPU when there are no messages
			Sleep( 1 );
		}
	}

	runSimThread.clear();
	simThread.join();

	delete _renderThread;
	delete _frameLimiter;
	return ( int ) msg.wParam;
}

LRESULT D3DAppFramework::MsgProc( HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_MOUSEMOVE: {
		INT32 x = GET_X_LPARAM( lParam );
		INT32 y = GET_Y_LPARAM( lParam );
		OnMouseInput( x, y );
	}
	return 0;

	case WM_INPUT: {
		UINT32 dwSize = 0U;

		GetRawInputData( ( HRAWINPUT ) lParam, RID_INPUT, NULL, &dwSize, sizeof( RAWINPUTHEADER ) );
		LPBYTE lpb = new BYTE[dwSize];
		if ( lpb != nullptr ) {
			INT32 readSize = GetRawInputData( ( HRAWINPUT ) lParam, RID_INPUT, lpb, &dwSize, sizeof( RAWINPUTHEADER ) ) ;

			if ( readSize != dwSize ) {
				FATALERROR( this, "GetRawInputData returned incorrect size" );
			} else {
				RAWINPUT* rawInput = ( RAWINPUT* ) lpb;
				OnRawInput( rawInput );
				delete [] lpb;
			}
		}
	}
	// Even if you handle the event, you have to call DefWindowProx after a WM_Input message so the system can perform cleanup
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms645590%28v=vs.85%29.aspx
	return DefWindowProc( hwnd, msg, wParam, lParam );

	// WM_SIZE is sent when the user resizes the window.
	case WM_SIZE:
		//TODO need member variable for size, so setting full screen doesn't overwrite it,
		_swapDesc.BufferDesc.Width  = LOWORD( lParam ) < Resources::MIN_SCREEN_X ? Resources::MIN_SCREEN_X : LOWORD( lParam );
		_swapDesc.BufferDesc.Height = HIWORD( lParam ) < Resources::MIN_SCREEN_Y ? Resources::MIN_SCREEN_Y : HIWORD( lParam );

		if ( _d3dDevice ) {
			if ( wParam == SIZE_MINIMIZED ) {
				_minimized = true;
				_maximized = false;
			} else if ( wParam == SIZE_MAXIMIZED ) {
				_maximized = true;
				_minimized = false;
				//tell the render thread to resize at the start of the next frame
				_resize.store( true );
			}
			// Restored is any resize that is not a minimize or maximize.
			// For example, restoring the window to its default size
			// after a minimize or maximize, or from dragging the resize
			// bars.
			else if ( wParam == SIZE_RESTORED ) {
				// Are we restoring from a mimimized or maximized state,
				if ( _minimized ) {
					_minimized = false;
					//tell the render thread to resize at the start of the next frame
					_resize.store( true );
				} else if ( _maximized ) {
					_maximized = false;
					//tell the render thread to resize at the start of the next frame
					_resize.store( true );
				} else if ( _resizing ) {
					// No, which implies the user is resizing by dragging
					// the resize bars.  However, we do not reset the device
					// here because as the user continuously drags the resize
					// bars, a stream of WM_SIZE messages is sent to the window,
					// and it would be pointless (and slow) to reset for each
					// WM_SIZE message received from dragging the resize bars.
					// So instead, we reset after the user is done resizing the
					// window and releases the resize bars, which sends a
					// WM_EXITSIZEMOVE message.
				} else { // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					//tell the render thread to resize at the start of the next frame
					_resize.store( true );
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
		_resize.store( true );
		return 0;

		// WM_CLOSE is sent when the user presses the 'X' button in the
		// caption bar menu. or when the MGDF system schedules a shutdown
	case WM_CLOSE:
		if ( _internalShutDown ) {
			//make sure we stop rendering before disposing of the window
			_runRenderThread.clear();
			_renderThread->join();
			//if we triggered this, then shut down
			DestroyWindow( _window );
		} else {
			//otherwise just inform the rest of the system that
			//it should shut down ASAP, but give it time to shut down cleanly
			ExternalClose();
		}
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;

	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT( 0, MNC_CLOSE );

	case WM_SYSKEYDOWN:
		switch ( wParam ) {
		case VK_F12: {
			//Toggle system stats overlay with alt f12
			bool exp = true;
			//if its true set it to false.
			if ( !_drawSystemOverlay.compare_exchange_weak( exp, false ) ) {
				//otherwise it must be false so set it to true
				_drawSystemOverlay.store( true );
			}
		}
		break;
		}
		return 0;
	default:
		return DefWindowProc( hwnd, msg, wParam, lParam );
	}
}

}
}
