#include "StdAfx.h"

#include <boost/lexical_cast.hpp>

#include "../common/MGDFResources.hpp"
#include "MGDFGraphicsManagerImpl.hpp"
#include "MGDFPreferenceConstants.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#define BACKBUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

namespace MGDF
{
namespace core
{

GraphicsManager::GraphicsManager( ID3D11Device *d3dDevice, ID2D1Device* d2dDevice, IDXGIAdapter1 *adapter )
	: _d3dDevice( d3dDevice )
	, _d2dDevice( d2dDevice )
	, _currentAdaptorMode( nullptr )
	, _currentMultiSampleLevel( 1 )
	, _backBufferMultiSampleLevel( 1 )
	, _vsync( true )
{
	_ASSERTE( d3dDevice );
	_ASSERTE( d2dDevice );
	_ASSERTE( adapter );

	IDXGIOutput *output;
	if ( FAILED( adapter->EnumOutputs( 0, &output ) ) ) {
		return;
	}

	_changePending.store( false );

	UINT32 maxAdaptorModes = 0U;
	if ( FAILED( output->GetDisplayModeList( BACKBUFFER_FORMAT, DXGI_ENUM_MODES_INTERLACED, &maxAdaptorModes, nullptr ) ) ) {
		SAFE_RELEASE( output );
		return;
	}

	DXGI_MODE_DESC *modes = new DXGI_MODE_DESC[maxAdaptorModes];
	if ( FAILED( output->GetDisplayModeList( BACKBUFFER_FORMAT, DXGI_ENUM_MODES_INTERLACED, &maxAdaptorModes, modes ) ) ) {
		SAFE_RELEASE( output );
		delete[] modes;
		return;
	}

	for ( UINT32 mode = 0; mode < maxAdaptorModes; ++mode ) {
		DXGI_MODE_DESC *displayMode	= &modes[mode];
		// Does this adaptor mode support  the desired format and is it above the minimum required resolution
		if ( displayMode->Format == BACKBUFFER_FORMAT && displayMode->Width >= Resources::MIN_SCREEN_X && displayMode->Height >= Resources::MIN_SCREEN_Y ) {
			GraphicsAdaptorMode *adaptorMode = new GraphicsAdaptorMode( displayMode->Width, displayMode->Height, displayMode->RefreshRate.Numerator, displayMode->RefreshRate.Denominator );
			_adaptorModes.Add( adaptorMode );
		}
	}

	delete[] modes;

	//determine the supported multisampling settings for this device
	for ( UINT32 i = 1; i < D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i ) {
		UINT32 quality = 0;
		if ( FAILED( _d3dDevice->CheckMultisampleQualityLevels( BACKBUFFER_FORMAT, i, &quality ) ) || quality == 0 ) {
			continue;
		}

		_multiSampleLevels.Add( i );
		_multiSampleQuality[i] = quality;
	}
	SAFE_RELEASE( output );
}

GraphicsManager::~GraphicsManager( void )
{
	for ( auto mode : *_adaptorModes.Items() ) {
		delete static_cast<GraphicsAdaptorMode *>( mode );
	}
}

IUIntList *GraphicsManager::GetMultiSampleLevels() const
{
	return ( IUIntList * ) &_multiSampleLevels;
}

bool GraphicsManager::SetCurrentMultiSampleLevel( UINT32 multisampleLevel )
{
	boost::mutex::scoped_lock lock( _mutex );
	if ( _multiSampleQuality.find( multisampleLevel ) != _multiSampleQuality.end() ) {
		_currentMultiSampleLevel = multisampleLevel;
		return true;
	} else {
		return false;
	}
}

UINT32 GraphicsManager::GetCurrentMultiSampleLevel( UINT32 *quality ) const
{
	if ( quality ) *quality = _multiSampleQuality.find( _currentMultiSampleLevel )->second - 1;
	return _currentMultiSampleLevel;
}

bool GraphicsManager::SetBackBufferMultiSampleLevel( UINT32 multisampleLevel )
{
	boost::mutex::scoped_lock lock( _mutex );
	if ( _multiSampleQuality.find( multisampleLevel ) != _multiSampleQuality.end() ) {
		_backBufferMultiSampleLevel = multisampleLevel;
		return true;
	} else {
		return false;
	}
}

UINT32 GraphicsManager::GetBackBufferMultiSampleLevel() const
{
	return _backBufferMultiSampleLevel;
}

bool GraphicsManager::GetVSync() const
{
	return _vsync;
}

void GraphicsManager::SetVSync( bool vsync )
{
	boost::mutex::scoped_lock lock( _mutex );
	_vsync = vsync;
}

const IGraphicsAdaptorModeList *GraphicsManager::GetAdaptorModes() const
{
	return &_adaptorModes;
}

IGraphicsAdaptorMode *GraphicsManager::GetAdaptorMode( UINT32 width, UINT32 height ) const
{
	IGraphicsAdaptorMode *mode = nullptr;
	for ( auto currentMode : *_adaptorModes.Items() ) {
		if ( currentMode->GetWidth() == width && currentMode->GetHeight() == height ) {
			if ( mode == nullptr ) {
				mode = currentMode;
			} else if (
			    currentMode->GetRefreshRateNumerator() >= mode->GetRefreshRateNumerator() &&
			    currentMode->GetRefreshRateDenominator() <= mode->GetRefreshRateDenominator() ) {
				mode = currentMode;
			}
		}
	}
	return mode;
}

IGraphicsAdaptorMode *GraphicsManager::GetCurrentAdaptorMode() const
{
	return _currentAdaptorMode;
}

UINT32 GraphicsManager::GetScreenX() const
{
	return _currentAdaptorMode->GetWidth();
}

UINT32 GraphicsManager::GetScreenY() const
{
	return _currentAdaptorMode->GetHeight();
}

void GraphicsManager::SetCurrentAdaptorMode( IGraphicsAdaptorMode *mode )
{
	boost::mutex::scoped_lock lock( _mutex );
	_currentAdaptorMode = mode;
}

void GraphicsManager::ApplyChanges()
{
	boost::mutex::scoped_lock lock( _mutex );
	_changePending.store( true );
}

bool GraphicsManager::IsBackBufferChangePending()
{
	bool exp = true;
	return _changePending.compare_exchange_weak( exp, true );
}

void GraphicsManager::SetBackBuffer( ID3D11Texture2D *backBuffer )
{
	_backBuffer = backBuffer;
}

ID3D11Texture2D *GraphicsManager::GetBackBuffer() const
{
	return _backBuffer;
}

void GraphicsManager::GetBackBufferDescription( D3D11_TEXTURE2D_DESC *desc ) const
{
	_backBuffer->GetDesc( desc );
}

ID3D11Device *GraphicsManager::GetD3DDevice() const
{
	return _d3dDevice;
}

bool GraphicsManager::SetBackBufferRenderTarget(ID2D1DeviceContext *context)
{
	D2D1_PIXEL_FORMAT pixelFormat;
	pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

	D2D1_BITMAP_PROPERTIES1 bitmapProperties;
	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	bitmapProperties.pixelFormat = pixelFormat;
	bitmapProperties.dpiX = 0;
	bitmapProperties.dpiX = 0;
	bitmapProperties.colorContext = nullptr;

	IDXGISurface1* dxgiSurface;
	if ( !FAILED(_backBuffer->QueryInterface<IDXGISurface1>(&dxgiSurface))) {
		ID2D1Bitmap1 *bitmap;
		if ( !FAILED(context->CreateBitmapFromDxgiSurface( dxgiSurface, bitmapProperties,&bitmap ))) {	
			context->SetTarget(bitmap);
			SAFE_RELEASE(bitmap);
			return true;
		}
		SAFE_RELEASE( dxgiSurface );
	}
	return false;
}

ID2D1Device *GraphicsManager::GetD2DDevice() const
{
	return _d2dDevice;
}

void GraphicsManager::LoadPreferences( IGame *game )
{
	bool savePreferences = false;
	_fullScreen = strcmp( "1", game->GetPreference( PreferenceConstants::FULL_SCREEN ) ) == 0;
	_vsync = strcmp( "1", game->GetPreference( PreferenceConstants::VSYNC ) ) == 0;

	if ( game->HasPreference( PreferenceConstants::SCREEN_X ) && game->HasPreference( PreferenceConstants::SCREEN_Y ) ) {
		_currentAdaptorMode = GetAdaptorMode(
		                          atoi( game->GetPreference( PreferenceConstants::SCREEN_X ) ),
		                          atoi( game->GetPreference( PreferenceConstants::SCREEN_Y ) ) );
	}

	if ( _currentAdaptorMode == nullptr ) {
		//set 1024*768 as the default
		_currentAdaptorMode = GetAdaptorMode( Resources::MIN_SCREEN_X, Resources::MIN_SCREEN_Y );
		//or if 1024*768 was unavailble the first adaptor mode in the list
		if ( _currentAdaptorMode == nullptr ) {
			_currentAdaptorMode = _adaptorModes.Get( 0 );
		}


		//try to find the native resolution if possible, otherwise stick to the default found above if none are found.
		INT32 nativeWidth = GetSystemMetrics( SM_CXSCREEN );
		INT32 nativeHeight = GetSystemMetrics( SM_CYSCREEN );
		for ( auto mode : *_adaptorModes.Items() ) {
			if ( mode->GetWidth() == nativeWidth && mode->GetHeight() == nativeHeight ) {
				_currentAdaptorMode = mode;
				break;
			}
		}

		game->SetPreference( PreferenceConstants::SCREEN_X, boost::lexical_cast<std::string> ( _currentAdaptorMode->GetWidth() ).c_str() );
		game->SetPreference( PreferenceConstants::SCREEN_Y, boost::lexical_cast<std::string> ( _currentAdaptorMode->GetHeight() ).c_str() );
		savePreferences = true;
	}

	//ensure the multisample level is not above what is supported.
	_currentMultiSampleLevel = atoi( game->GetPreference( PreferenceConstants::RT_MULTISAMPLE_LEVEL ) );
	if ( _currentMultiSampleLevel > _multiSampleLevels.Get( _multiSampleLevels.Size() - 1 ) ) {
		_currentMultiSampleLevel = _multiSampleLevels.Get( _multiSampleLevels.Size() - 1 );
		game->SetPreference( PreferenceConstants::RT_MULTISAMPLE_LEVEL, boost::lexical_cast<std::string> ( _currentMultiSampleLevel ).c_str() );
		savePreferences = true;
	}
	_backBufferMultiSampleLevel = atoi( game->GetPreference( PreferenceConstants::MULTISAMPLE_LEVEL ) );
	if ( _backBufferMultiSampleLevel > _multiSampleLevels.Get( _multiSampleLevels.Size() - 1 ) ) {
		_backBufferMultiSampleLevel = _multiSampleLevels.Get( _multiSampleLevels.Size() - 1 );
		game->SetPreference( PreferenceConstants::MULTISAMPLE_LEVEL, boost::lexical_cast<std::string> ( _backBufferMultiSampleLevel ).c_str() );
		savePreferences = true;
	}

	if ( savePreferences ) {
		game->SavePreferences();
	}
}

void GraphicsManager::OnResetSwapChain( DXGI_SWAP_CHAIN_DESC *desc, BOOL *fullScreen )
{
	boost::mutex::scoped_lock lock( _mutex );

	if ( fullScreen != nullptr ) {
		_fullScreen = *fullScreen != 0;
	}

	desc->Windowed = !_fullScreen;

	desc->BufferDesc.Width  = _currentAdaptorMode->GetWidth();
	desc->BufferDesc.Height = _currentAdaptorMode->GetHeight();
	desc->BufferDesc.RefreshRate.Numerator = _vsync ? _currentAdaptorMode->GetRefreshRateNumerator() : 0;
	desc->BufferDesc.RefreshRate.Denominator = _vsync ? _currentAdaptorMode->GetRefreshRateDenominator() : 1;

	desc->BufferDesc.Format = BACKBUFFER_FORMAT;
	desc->BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc->BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	desc->SampleDesc.Count = _backBufferMultiSampleLevel;
	desc->SampleDesc.Quality = _multiSampleQuality[_backBufferMultiSampleLevel] - 1;
	desc->BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc->BufferCount = 1;
	desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	_changePending.store( false );
}

}
}