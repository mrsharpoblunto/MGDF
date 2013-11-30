#include "StdAfx.h"

#include "MGDFApp.hpp"
#include "core.impl/MGDFSystemBuilder.hpp"
#include "core.impl/MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

MGDFApp::MGDFApp( HINSTANCE hInstance )
	: D3DAppFramework( hInstance )
	, _system( nullptr )
	, _blackBrush( nullptr )
	, _whiteBrush( nullptr )
	, _textFormat( nullptr )
	, _dWriteFactory( nullptr )
	, _initialized( false )
{
}

MGDFApp::~MGDFApp()
{
	SAFE_RELEASE(_blackBrush);
	SAFE_RELEASE(_whiteBrush);
	SAFE_RELEASE(_textFormat);
	SAFE_RELEASE(_dWriteFactory);
}

void MGDFApp::SetSystem( System *system )
{
	_ASSERTE( system );
	_system = system;
	_system->AddShutDownCallback( [this]() {
		_internalShutDown = true;
		PostMessage( _window, WM_CLOSE, 0, 0 );
	} );
}

void MGDFApp::InitDirect3D( const std::string &caption, WNDPROC windowProcedure )
{
	//find out what D3D feature levels this module supports
	D3D_FEATURE_LEVEL *levels = nullptr;
	UINT32 featureLevelsSize = 0;
	if ( _system->GetCompatibleD3DFeatureLevels( levels, &featureLevelsSize ) ) {
		levels = new D3D_FEATURE_LEVEL[featureLevelsSize];
		_system->GetCompatibleD3DFeatureLevels( levels, &featureLevelsSize );
	}

	D3DAppFramework::InitDirect3D( caption, windowProcedure, levels, featureLevelsSize );

	delete[] levels;

	_system->SetDevices( _d3dDevice, _d2dDevice );  //allow the system to pass the d3d object to the modules

	if ( FAILED( DWriteCreateFactory(
	            DWRITE_FACTORY_TYPE_SHARED,
	            __uuidof( IDWriteFactory1 ),
	            reinterpret_cast<IUnknown**>( &_dWriteFactory )
	        ) ) ) {
		FATALERROR( _system, "Unable to create IDWriteFactory" );
	}

	IDWriteFontCollection *fontCollection;
	if (FAILED(_dWriteFactory->GetSystemFontCollection( &fontCollection ))) {
		FATALERROR( _system, "Unable to get system font collection" );
	}

	if (FAILED(_dWriteFactory->CreateTextFormat( 
		L"Arial",
		fontCollection,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		14,L"",&_textFormat))) {
		FATALERROR( _system, "Unable to create text format" );
	}

	D2D1_COLOR_F color;
	color.a = color.r = color.g = color.b = 1.0f;
	if (FAILED(_context->CreateSolidColorBrush(color,&_whiteBrush))) {
		FATALERROR(_system,"Unable to create white color brush");
	}

	color.r = color.g = color.b = 0.0f;
	color.a = 0.85f;
	if (FAILED(_context->CreateSolidColorBrush(color,&_blackBrush))) {
		FATALERROR(_system,"Unable to create black color brush");
	}

	SAFE_RELEASE(fontCollection);
}

void MGDFApp::UpdateScene( double simulationTime )
{
	LARGE_INTEGER simulationStart = _timer.GetCurrentTimeTicks();

	if ( !_initialized ) {
		_system->Initialize();
		_initialized = true;
	}

	//execute one frame of game logic as per the current module
	_system->UpdateScene( simulationTime, _stats );

	LARGE_INTEGER simulationEnd = _timer.GetCurrentTimeTicks();

	_stats.AppendActiveSimTime( _timer.ConvertDifferenceToSeconds( simulationEnd, simulationStart ) );
}

void MGDFApp::OnRawInput( RAWINPUT *input )
{
	_system->GetInputManagerImpl()->HandleInput( input );
}

void MGDFApp::OnMouseInput( INT32 x, INT32 y )
{
	_system->GetInputManagerImpl()->HandleInput( x, y );
}

void MGDFApp::OnInit(  ID3D11Device *d3dDevice, ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter )
{
	_system->CreateGraphicsImpl( d3dDevice, d2dDevice, adapter );
}

bool MGDFApp::IsBackBufferChangePending()
{
	return _system->GetGraphicsImpl()->IsBackBufferChangePending();
}

void MGDFApp::OnResetSwapChain( DXGI_SWAP_CHAIN_DESC *swapDesc, BOOL *doFullScreen )
{
	_system->GetGraphicsImpl()->OnResetSwapChain( swapDesc, doFullScreen );
}

void MGDFApp::OnBackBufferChanged( ID3D11Texture2D *backBuffer )
{
	_system->GetGraphicsImpl()->SetBackBuffer( backBuffer );
	_system->GetGraphics()->SetBackBufferRenderTarget(_context);
	_system->BackBufferChanged();
}

void MGDFApp::OnInputIdle()
{
	_system->GetInputManagerImpl()->ProcessInput();
}

void MGDFApp::DrawScene( double alpha )
{
	_system->DrawScene( alpha );  //render as per the current active module

	bool exp = true;
	if ( _drawSystemOverlay.compare_exchange_weak( exp, true ) ) {
		DrawSystemOverlay();
	}
}

void MGDFApp::DrawSystemOverlay()
{
	const float margin = 5.0f;

	std::wstring information;
	std::string info = _system->GetSystemInformation( _stats );
	information.assign( info.begin(), info.end() );

	IDWriteTextLayout *textLayout;
	if (FAILED(_dWriteFactory->CreateTextLayout(
		information.c_str(),
		static_cast<UINT32>(info.size()),
		_textFormat,
		static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetWidth()),
		static_cast<float>(_system->GetGraphics()->GetCurrentAdaptorMode()->GetHeight()),
		&textLayout))) {
		FATALERROR(_system,"Unable to create text layout");
	}

	DWRITE_TEXT_METRICS metrics;
	ZeroMemory(&metrics,sizeof(metrics));
	if (FAILED(textLayout->GetMetrics(&metrics))) {
		FATALERROR(_system,"Unable to get text overhang metrics");
	}

	_context->BeginDraw();

	D2D1_ROUNDED_RECT rect;
	rect.radiusX = margin;
	rect.radiusY = margin;
	rect.rect.top = margin;
	rect.rect.left = margin;
	rect.rect.bottom = ( margin * 3 ) + metrics.height;
	rect.rect.right = ( margin * 3 ) + metrics.width;
	_context->FillRoundedRectangle(&rect,_blackBrush);
	_context->DrawRoundedRectangle(&rect,_whiteBrush);

	D2D_POINT_2F origin;
	origin.x = ( 2 * margin );
	origin.y = ( 2 * margin );
	_context->DrawTextLayout(origin,textLayout,_whiteBrush);

	_context->EndDraw();

	SAFE_RELEASE(textLayout);
}

void MGDFApp::FatalError( const char *sender, const char *message )
{
	_system->FatalError( sender, message );
}

void MGDFApp::ExternalClose()
{
	_system->QueueShutDown();
}

}
}