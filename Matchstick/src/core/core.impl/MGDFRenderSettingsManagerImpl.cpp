#include "StdAfx.h"

#include <sstream>

#include "../common/MGDFResources.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFRenderSettingsManagerImpl.hpp"
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

std::string ToString( UINT32 i )
{
	std::ostringstream ss;
	ss << i;
	return ss.str();
}

RenderSettingsManager::RenderSettingsManager( )
	: _currentMultiSampleLevel( 1 )
	, _backBufferMultiSampleLevel( 1 )
	, _vsync( true )
	, _screenX( 0 )
	, _screenY( 0 )
{
	ZeroMemory( &_currentAdaptorMode, sizeof( AdaptorMode ) );
}

void RenderSettingsManager::InitFromDevice( ID3D11Device *d3dDevice, IDXGIAdapter1 *adapter )
{
	_ASSERTE( d3dDevice );
	_ASSERTE( adapter );

	std::lock_guard<std::mutex> lock( _mutex );
	Cleanup();

	IDXGIOutput *temp;
	if ( FAILED( adapter->EnumOutputs( 0, &temp ) ) ) {
		return;
	}
	IDXGIOutput1 *output;
	if ( FAILED( temp->QueryInterface( __uuidof(IDXGIOutput1), (void **)&output ) ) ) {
		return;
	}
	SAFE_RELEASE( temp );

	UINT32 maxAdaptorModes = 0U;
	if ( FAILED( output->GetDisplayModeList1( BACKBUFFER_FORMAT, 0, &maxAdaptorModes, nullptr ) ) ) {
		SAFE_RELEASE( output );
		return;
	}

	DXGI_MODE_DESC1 *modes = new DXGI_MODE_DESC1[maxAdaptorModes];
	if ( FAILED( output->GetDisplayModeList1( BACKBUFFER_FORMAT, 0, &maxAdaptorModes, modes ) ) ) {
		SAFE_RELEASE( output );
		delete[] modes;
		return;
	}

	bool foundMatchingCurrentAdaptor = false;

	for ( UINT32 mode = 0; mode < maxAdaptorModes; ++mode ) {
		DXGI_MODE_DESC1 *displayMode = &modes[mode];
		// Does this adaptor mode support  the desired format and is it above the minimum required resolution
		if ( displayMode->Format == BACKBUFFER_FORMAT && 
				!displayMode->Stereo && // Stereo adapters not currently supported
				displayMode->Width >= Resources::MIN_SCREEN_X && 
				displayMode->Height >= Resources::MIN_SCREEN_Y ) {

			LOG( "Found valid adapter mode " << displayMode->Width << "x" << displayMode->Height, LOG_MEDIUM );
			AdaptorMode adaptorMode;
			adaptorMode.Width = displayMode->Width;
			adaptorMode.Height = displayMode->Height;
			adaptorMode.RefreshRateNumerator = displayMode->RefreshRate.Numerator;
			adaptorMode.RefreshRateDenominator = displayMode->RefreshRate.Denominator;
			_adaptorModes.push_back( adaptorMode );

			// try to preserve the current adaptor mode settings when devices change
			if ( _currentAdaptorMode.Width == adaptorMode.Width &&
				_currentAdaptorMode.Height == adaptorMode.Height &&
				_currentAdaptorMode.RefreshRateNumerator == adaptorMode.RefreshRateNumerator &&
				_currentAdaptorMode.RefreshRateDenominator == adaptorMode.RefreshRateDenominator ) 
			{
				_currentAdaptorMode = adaptorMode;
				foundMatchingCurrentAdaptor = true;
			}
		}
	}

	// ensure we always have a current adaptor mode set
	if ( !foundMatchingCurrentAdaptor ) {
		_currentAdaptorMode = _adaptorModes.at( 0 );
	}

	delete[] modes;

	//determine the supported multisampling settings for this device
	for ( UINT32 i = 1; i < D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i ) {
		UINT32 quality = 0;
		if ( FAILED( d3dDevice->CheckMultisampleQualityLevels( BACKBUFFER_FORMAT, i, &quality ) ) || quality == 0 ) {
			continue;
		}
		LOG( "Found valid multisample level " << i , LOG_MEDIUM );
		_multiSampleLevels.push_back( i );
		_multiSampleQuality[i] = quality;
	}
	SAFE_RELEASE( output );
}

RenderSettingsManager::~RenderSettingsManager( void )
{
	Cleanup();
}

UINT32 RenderSettingsManager::GetMultiSampleLevelCount() const 
{
	std::lock_guard<std::mutex> lock( _mutex );
	return static_cast<UINT32>( _multiSampleLevels.size() );
}

bool RenderSettingsManager::GetMultiSampleLevel( UINT32 index, UINT32 * level ) const
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( !level ) return false;
	if ( index < _multiSampleLevels.size() ) {
		*level = _multiSampleLevels[index];
		return true;
	}
	return false;
}

bool RenderSettingsManager::SetCurrentMultiSampleLevel( UINT32 multisampleLevel )
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _multiSampleQuality.find( multisampleLevel ) != _multiSampleQuality.end() ) {
		_currentMultiSampleLevel = multisampleLevel;
		return true;
	} else {
		return false;
	}
}

UINT32 RenderSettingsManager::GetCurrentMultiSampleLevel( UINT32 *quality ) const
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( quality ) *quality = _multiSampleQuality.find( _currentMultiSampleLevel )->second - 1;
	return _currentMultiSampleLevel;
}

bool RenderSettingsManager::SetBackBufferMultiSampleLevel( UINT32 multisampleLevel )
{
	std::lock_guard<std::mutex> lock( _mutex );
	if ( _multiSampleQuality.find( multisampleLevel ) != _multiSampleQuality.end() ) {
		_backBufferMultiSampleLevel = multisampleLevel;
		return true;
	} else {
		return false;
	}
}

UINT32 RenderSettingsManager::GetBackBufferMultiSampleLevel() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _backBufferMultiSampleLevel;
}

bool RenderSettingsManager::GetVSync() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _vsync;
}

void RenderSettingsManager::SetVSync( bool vsync )
{
	std::lock_guard<std::mutex> lock( _mutex );
	_vsync = vsync;
}

bool RenderSettingsManager::GetFullscreen() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _fullScreen;
}

void RenderSettingsManager::SetFullscreen( bool fullScreen )
{
	std::lock_guard<std::mutex> lock( _mutex );
	_fullScreen = fullScreen;
}

UINT32 RenderSettingsManager::GetAdaptorModeCount() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return static_cast<UINT32>( _adaptorModes.size() );
}

bool RenderSettingsManager::GetAdaptorMode( UINT32 index, AdaptorMode * mode ) const
{
	if ( !mode ) return false;
	std::lock_guard<std::mutex> lock( _mutex );
	if ( index < _adaptorModes.size() ) {
		auto m = _adaptorModes.at(index);
		mode->Width = m.Width;
		mode->Height = m.Height;
		mode->RefreshRateNumerator = m.RefreshRateNumerator;
		mode->RefreshRateDenominator = m.RefreshRateDenominator;
		return true;
	}
	return false;
}

bool RenderSettingsManager::GetAdaptorMode( UINT32 width, UINT32 height, AdaptorMode * mode ) const
{
	if ( !mode ) return false;
	std::lock_guard<std::mutex> lock( _mutex );
	bool result = false;
	for ( auto currentMode : _adaptorModes ) {
		if ( currentMode.Width == width && currentMode.Height == height ) {
			if ( !result ) {
				mode->Width = currentMode.Width;
				mode->Height = currentMode.Height;
				mode->RefreshRateNumerator = currentMode.RefreshRateNumerator;
				mode->RefreshRateDenominator = currentMode.RefreshRateDenominator;
				result = true;
			} else if (
			    currentMode.RefreshRateNumerator >= mode->RefreshRateNumerator &&
			    currentMode.RefreshRateDenominator <= mode->RefreshRateDenominator ) {
				mode->RefreshRateNumerator = currentMode.RefreshRateNumerator;
				mode->RefreshRateDenominator = currentMode.RefreshRateDenominator;
			}
		}
	}
	return result;
}

void RenderSettingsManager::GetCurrentAdaptorMode( AdaptorMode * mode ) const
{
	if ( !mode ) return;
	std::lock_guard<std::mutex> lock( _mutex );
	mode->Width = _currentAdaptorMode.Width;
	mode->Height = _currentAdaptorMode.Height;
	mode->RefreshRateNumerator = _currentAdaptorMode.RefreshRateNumerator;
	mode->RefreshRateDenominator = _currentAdaptorMode.RefreshRateDenominator;
}

UINT32 RenderSettingsManager::GetScreenX() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _screenX;
}

UINT32 RenderSettingsManager::GetScreenY() const
{
	std::lock_guard<std::mutex> lock( _mutex );
	return _screenY;
}

bool RenderSettingsManager::SetCurrentAdaptorMode( const AdaptorMode *mode )
{
	if (!mode) return false;
	std::lock_guard<std::mutex> lock( _mutex );

	for ( auto currentMode : _adaptorModes ) {
		if ( currentMode.Width == mode->Width && 
			currentMode.Height == mode->Height &&
			currentMode.RefreshRateDenominator == mode->RefreshRateDenominator && 
			currentMode.RefreshRateNumerator == mode->RefreshRateNumerator ) {
			_currentAdaptorMode = currentMode;
			return true;
		}
	}

	return false;
}

void RenderSettingsManager::ApplyChanges()
{
	std::lock_guard<std::mutex> lock( _mutex );
	_changePending.store( true );
}

bool RenderSettingsManager::IsBackBufferChangePending()
{
	bool exp = true;
	return _changePending.compare_exchange_weak( exp, true );
}

void RenderSettingsManager::OnSwitchToFullScreen( DXGI_MODE_DESC1 &desc )
{
	std::lock_guard<std::mutex> lock( _mutex );

	desc.Width  = _currentAdaptorMode.Width;
	desc.Height = _currentAdaptorMode.Height;
	desc.RefreshRate.Numerator = _currentAdaptorMode.RefreshRateNumerator;
	desc.RefreshRate.Denominator = _currentAdaptorMode.RefreshRateDenominator;
	desc.Format = BACKBUFFER_FORMAT;
	desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.Stereo = false;

	_fullScreen = true;
	_screenX = desc.Width;
	_screenY = desc.Height;
}

void RenderSettingsManager::OnResize( UINT32 width, UINT32 height )
{
	std::lock_guard<std::mutex> lock( _mutex );

	_screenX = width;
	_screenY = height;
}

void RenderSettingsManager::OnResetSwapChain( DXGI_SWAP_CHAIN_DESC1 &desc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC& fullscreenDesc, const RECT& windowSize )
{
	std::lock_guard<std::mutex> lock( _mutex );

	fullscreenDesc.Windowed = !_fullScreen;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	fullscreenDesc.RefreshRate.Numerator = _currentAdaptorMode.RefreshRateNumerator;
	fullscreenDesc.RefreshRate.Denominator = _currentAdaptorMode.RefreshRateDenominator;

	desc.Width  = fullscreenDesc.Windowed ? windowSize.right : _currentAdaptorMode.Width;
	desc.Height = fullscreenDesc.Windowed ? windowSize.bottom : _currentAdaptorMode.Height;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	desc.Format = BACKBUFFER_FORMAT;
	desc.SampleDesc.Count = _backBufferMultiSampleLevel;
	desc.SampleDesc.Quality = _multiSampleQuality[_backBufferMultiSampleLevel] - 1;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.Stereo = false;
	
	_screenX = desc.Width;
	_screenY = desc.Height;

	_changePending.store( false );
}

void RenderSettingsManager::Cleanup()
{
	_changePending.store( false );
	_adaptorModes.clear();
	_multiSampleLevels.clear();
	_multiSampleQuality.clear();
}

void RenderSettingsManager::LoadPreferences( IGame *game )
{
	_ASSERTE( game );
	bool hasCurrentMode = false;

	bool savePreferences = false;
	_fullScreen = strcmp( "1", game->GetPreference( PreferenceConstants::FULL_SCREEN ) ) == 0;
	_vsync = strcmp( "1", game->GetPreference( PreferenceConstants::VSYNC ) ) == 0;

	if ( game->HasPreference( PreferenceConstants::SCREEN_X ) && game->HasPreference( PreferenceConstants::SCREEN_Y ) ) {
		hasCurrentMode = GetAdaptorMode(
		    atoi( game->GetPreference( PreferenceConstants::SCREEN_X ) ),
		    atoi( game->GetPreference( PreferenceConstants::SCREEN_Y ) ),
			&_currentAdaptorMode );
	}

	if ( !hasCurrentMode ) {
		//set 1024*768 as the default
		hasCurrentMode = GetAdaptorMode( Resources::MIN_SCREEN_X, Resources::MIN_SCREEN_Y, &_currentAdaptorMode );
		//or if 1024*768 was unavailble the first adaptor mode in the list
		if ( !hasCurrentMode ) {
			_currentAdaptorMode = _adaptorModes.at( 0 );
		}

		//try to find the native resolution if possible, otherwise stick to the default found above if none are found.
		INT32 nativeWidth = GetSystemMetrics( SM_CXSCREEN );
		INT32 nativeHeight = GetSystemMetrics( SM_CYSCREEN );
		for ( auto mode : _adaptorModes ) {
			if ( mode.Width == nativeWidth && mode.Height == nativeHeight ) {
				_currentAdaptorMode = mode;
				break;
			}
		}

		LOG( "No screen resolution preferences found, using " << _currentAdaptorMode.Width << "x" << _currentAdaptorMode.Height, LOG_LOW );
		game->SetPreference( PreferenceConstants::SCREEN_X, ToString( _currentAdaptorMode.Width ).c_str() );
		game->SetPreference( PreferenceConstants::SCREEN_Y, ToString( _currentAdaptorMode.Height ).c_str() );
		savePreferences = true;
	} else {
		LOG( "Loaded screen resolution preference for " << _currentAdaptorMode.Width << "x" << _currentAdaptorMode.Height, LOG_LOW );
	}

	//ensure the multisample level is not above what is supported.
	_currentMultiSampleLevel = atoi( game->GetPreference( PreferenceConstants::RT_MULTISAMPLE_LEVEL ) );
	if ( _currentMultiSampleLevel > _multiSampleLevels.at( _multiSampleLevels.size() - 1 ) ) {
		_currentMultiSampleLevel = _multiSampleLevels.at( _multiSampleLevels.size() - 1 );
		LOG( "RT multisample preference is not supported, using " << _currentMultiSampleLevel << " instead", LOG_LOW );
		game->SetPreference( PreferenceConstants::RT_MULTISAMPLE_LEVEL, ToString( _currentMultiSampleLevel ).c_str() );
		savePreferences = true;
	}
	_backBufferMultiSampleLevel = atoi( game->GetPreference( PreferenceConstants::MULTISAMPLE_LEVEL ) );
	if ( _backBufferMultiSampleLevel > _multiSampleLevels.at( _multiSampleLevels.size() - 1 ) ) {
		_backBufferMultiSampleLevel = _multiSampleLevels.at( _multiSampleLevels.size() - 1 );
		LOG( "Backbuffer multisample preference is not supported, using " << _backBufferMultiSampleLevel << " instead", LOG_LOW );
		game->SetPreference( PreferenceConstants::MULTISAMPLE_LEVEL, ToString( _backBufferMultiSampleLevel ).c_str() );
		savePreferences = true;
	}

	if ( savePreferences ) {
		game->SavePreferences();
	}
}

}
}