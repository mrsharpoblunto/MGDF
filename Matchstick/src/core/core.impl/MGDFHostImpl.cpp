#include "StdAfx.h"

#include <iomanip>
#include <filesystem>

#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "MGDFHostImpl.hpp"
#include "MGDFParameterConstants.hpp"
#include "MGDFPreferenceConstants.hpp"
#include "MGDFComponents.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"

#include "../vfs/archive/zip/ZipArchiveHandlerImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#define PENDING_SAVE_PREFIX "__"

using namespace std::tr2::sys;

namespace MGDF
{
namespace core
{


void IHostImpl::SetShutDownHandler( const ShutDownFunction handler )
{
	_shutDownHandler = handler;
}

void IHostImpl::SetFatalErrorHandler( const FatalErrorFunction handler )
{
	_fatalErrorHandler = handler;
}

MGDFError Host::TryCreate( Game *game, Host **host )
{
	*host = new Host( game );
	MGDFError error = (*host)->Init();
	if ( MGDF_OK != error ) {
		delete *host;
		*host = nullptr;
	}
	return error;
}

Host::Host( Game *game )
	: _game( game )
	, _timer( nullptr )
	, _saves( nullptr )
	, _module( nullptr )
	, _version( VersionHelper::Create( MGDFVersionInfo::MGDF_VERSION() ) )
	, _storage( Components::Instance().Get<storage::IStorageFactoryComponent>() )
	, _input( Components::Instance().Get<input::IInputManagerComponent>() )
	, _sound( Components::Instance().Get<audio::ISoundManagerComponent>() )
	, _vfs( Components::Instance().Get<vfs::IVirtualFileSystemComponent>() )
	, _stats( new StatisticsManager() )
	, _d3dDevice( nullptr )
	, _d3dContext( nullptr )
	, _d2dDevice( nullptr )
	, _backBuffer( nullptr )
{
	_shutdownQueued.store( false );
	_ASSERTE( game );
}

MGDFError Host::Init()
{
	LOG( "Creating Module factory...", LOG_LOW );
	MGDFError error = ModuleFactory::TryCreate( &_moduleFactory );
	if ( MGDF_OK != error ) return error;

	error = Timer::TryCreate( TIMER_SAMPLES, &_timer );
	if (MGDF_OK != error ) return error;

	//map essential directories to the vfs
	//ensure the vfs automatically enumerates zip files
	LOG( "Registering Zip file VFS handler...", LOG_LOW );
	_vfs->RegisterArchiveHandler( vfs::zip::CreateZipArchiveHandlerImpl( this ) );

	//ensure the vfs enumerates any custom defined archive formats
	LOG( "Registering custom archive VFS handlers...", LOG_LOW );
	UINT32 length = 0;
	_moduleFactory->GetCustomArchiveHandlers( nullptr, &length, &Logger::Instance(), this );
	if ( length > 0 ) {
		IArchiveHandler **handlers = new IArchiveHandler *[length];
		_moduleFactory->GetCustomArchiveHandlers( handlers, &length, &Logger::Instance(), this );
		for ( UINT32 i = 0; i < length; ++i )  {
			_vfs->RegisterArchiveHandler( handlers[i] );
		}
		delete[] handlers;
		LOG( "Registered " << length << " custom archive VFS handlers", LOG_LOW );
	} else {
		LOG( "No custom archive VFS handlers to be registered", LOG_LOW );
	}

	//enumerate the current games content directory
	LOG( "Mounting content directory into VFS...", LOG_LOW );
	_vfs->Mount( Resources::Instance().ContentDir().c_str() );

	//set the initial sound volumes
	if ( _sound != nullptr ) {
		LOG( "Setting initial volume...", LOG_HIGH );
		_sound->SetSoundVolume( ( float ) atof( _game->GetPreference( PreferenceConstants::SOUND_VOLUME ) ) );
		_sound->SetStreamVolume( ( float ) atof( _game->GetPreference( PreferenceConstants::MUSIC_VOLUME ) ) );
	}

	LOG( "Initialised host components successfully", LOG_LOW );
	return MGDF_OK;
}

Host::~Host( void )
{
	SAFE_RELEASE( _d3dContext );

	if ( _saves != nullptr ) {
		for ( auto save : *_saves->Items() ) {
			delete[] save;
		}
		delete _saves;
	}
	delete _timer;
	delete _game;
	delete _stats;
	delete _moduleFactory;

	LOG( "Uninitialised host successfully", LOG_LOW );
}

void Host::GetHostInfo( const HostStats &stats, std::wstringstream &ss ) const
{
	std::wstring mgdfVersion( MGDFVersionInfo::MGDF_VERSION().begin(), MGDFVersionInfo::MGDF_VERSION().end() );

	Timings timings;
	stats.GetTimings( timings );

	ss.setf( std::ios::fixed );
	ss.precision( 4 );

	ss << "MGDF Version: " << mgdfVersion << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";
	ss << "\r\nPerformance Statistics:\r\n";

	ss << "Render Thread\r\n";
	ss << " FPS : ";
	if ( timings.AvgRenderTime == 0 )
		ss << "N/A\r\n";
	else
		ss << 1 / timings.AvgRenderTime << "\r\n";
	ss << " Render CPU : " << timings.AvgActiveRenderTime << "\r\n";
	ss << " Idle CPU : " << timings.AvgRenderTime - timings.AvgActiveRenderTime << "\r\n";

	ss << "\r\nSim Thread\r\n";
	ss << " Expected FPS : ";
	if ( timings.ExpectedSimTime == 0 )
		ss << "N/A\r\n";
	else
		ss << 1 / timings.ExpectedSimTime << "\r\n";

	ss << " Actual FPS : ";
	if ( timings.AvgSimTime == 0 )
		ss << "N/A\r\n";
	else
		ss << 1 / timings.AvgSimTime << "\r\n";

	ss << " Input CPU : " << timings.AvgSimInputTime << "\r\n";
	ss << " Audio CPU : " << timings.AvgSimAudioTime << "\r\n";
	ss << " Other CPU : " << timings.AvgActiveSimTime << "\r\n";
	ss << " Idle CPU : " << ( timings.AvgSimTime - timings.AvgActiveSimTime - timings.AvgSimInputTime - timings.AvgSimAudioTime ) << "\r\n";

	_timer->GetCounterInformation( ss );
}

RenderSettingsManager &Host::GetRenderSettingsImpl()
{
	return _renderSettings;
}

input::IInputManagerComponent &Host::GetInputManagerImpl() const
{
	return *_input;
}

UINT32 Host::GetCompatibleD3DFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *featureLevelsSize )
{
	return _moduleFactory->GetCompatibleFeatureLevels( levels, featureLevelsSize );
}

/**
create and initialize a new module
*/
void Host::STCreateModule()
{
	if ( _module == nullptr ) {
		std::string error;
		if ( _moduleFactory->GetLastError( error ) ) {
			FATALERROR( this, error );
		}

		if ( !_moduleFactory->IsCompatibleInterfaceVersion( MGDFVersionInfo::MGDF_INTERFACE_VERSION ) ) {
			FATALERROR( this, "Module is not compatible with MGDF Interface version " << MGDFVersionInfo::MGDF_INTERFACE_VERSION );
		}

		//create the module
		_module = _moduleFactory->GetModule();

		if ( _module == nullptr ) {
			FATALERROR( this, "Unable to create module class" );
		}

		//init the module
		if ( !_module->STNew( this, Resources::Instance().WorkingDir().c_str() ) ) {
			FATALERROR( this, "Error initialising module" );
		}
	}
}

void Host::STUpdate( double simulationTime, HostStats &stats )
{
	bool exp = true;
	if ( _module != nullptr && _shutdownQueued.compare_exchange_strong( exp, false ) ) {
		LOG( "Calling module STShutDown...", LOG_MEDIUM );
		_module->STShutDown( this );
	}

	LARGE_INTEGER inputStart = _timer->GetCurrentTimeTicks();
	_input->ProcessSim();
	LARGE_INTEGER inputEnd = _timer->GetCurrentTimeTicks();

	LARGE_INTEGER audioStart = _timer->GetCurrentTimeTicks();
	if ( _sound != nullptr ) _sound->Update();
	LARGE_INTEGER audioEnd = _timer->GetCurrentTimeTicks();

	stats.AppendSimInputAndAudioTimes(
	    _timer->ConvertDifferenceToSeconds( inputEnd, inputStart ),
	    _timer->ConvertDifferenceToSeconds( audioEnd, audioStart ) );

	if ( _module != nullptr ) {
		LOG( "Calling module STUpdate...", LOG_HIGH );
		if ( !_module->STUpdate( this, simulationTime ) ) {
			FATALERROR( this, "Error updating scene in module" );
		}
	}
}

void Host::STDisposeModule()
{
	LOG( "Calling module STDispose...", LOG_MEDIUM );
	if ( _module != nullptr && !_module->STDispose( this ) ) {
		_module = nullptr;
		FATALERROR( this, "Error disposing module" );
	}
	LOG( "Disposed of module successfully", LOG_LOW );
}

void Host::RTBeforeFirstDraw()
{
	if ( _module != nullptr ) {
		LOG( "Calling module RTBeforeFirstDraw...", LOG_MEDIUM );
		if ( !_module->RTBeforeFirstDraw( this ) ) {
			FATALERROR( this, "Error in before first draw in module" );
		}
	}
}

void Host::RTBeforeDeviceReset()
{
	SAFE_RELEASE( _d3dContext );
	if ( _module != nullptr ) {
		LOG( "Calling module RTBeforeDeviceReset...", LOG_MEDIUM );
		if ( !_module->RTBeforeDeviceReset( this ) ) {
			FATALERROR( this, "Error in before device reset in module" );
		}
	}
}

void Host::RTSetDevices( HWND window, ID3D11Device *d3dDevice, ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter )
{
	LOG( "Initializing render settings and GPU timers...", LOG_LOW );
	_renderSettings.InitFromDevice( window, d3dDevice, adapter );
	_timer->InitFromDevice( d3dDevice, GPU_TIMER_BUFFER );

	if ( _renderSettings.GetAdaptorModeCount() == 0 ) {
		FATALERROR( this, "No compatible adaptor modes found" );
	}

	if ( !_d3dDevice ) {
		LOG( "Loading Render settings...", LOG_LOW );
		_renderSettings.LoadPreferences( _game );
	}

	_d2dDevice = d2dDevice;
	_d3dDevice = d3dDevice;
	_d3dDevice->GetImmediateContext( &_d3dContext );
	_ASSERTE( _d3dContext );
}

void Host::RTDraw( double alpha )
{
	_timer->Begin();
	if ( _module != nullptr ) {
		LOG( "Calling module RTDraw...", LOG_HIGH );
		if ( !_module->RTDraw( this, alpha ) ) {
			FATALERROR( this, "Error drawing scene in module" );
		}
	}
	_timer->End();
}

void Host::RTBeforeBackBufferChange()
{
	if ( _module != nullptr ) {
		LOG( "Calling module RTBeforeBackBufferChange...", LOG_MEDIUM );
		if ( !_module->RTBeforeBackBufferChange( this ) ) {
			FATALERROR( this, "Error handling before back buffer change in module" );
		}
	}
}

void Host::RTBackBufferChange( ID3D11Texture2D *backBuffer )
{
	_backBuffer = backBuffer;
	if ( _module != nullptr ) {
		LOG( "Calling module RTBackBufferChange...", LOG_MEDIUM );
		if ( !_module->RTBackBufferChange( this ) ) {
			FATALERROR( this, "Error handling back buffer change in module" );
		}
	}
}

ID3D11Texture2D *Host::GetBackBuffer() const
{
	return _backBuffer;
}

void Host::GetBackBufferDescription( D3D11_TEXTURE2D_DESC *desc ) const
{
	_backBuffer->GetDesc( desc );
}

ID3D11Device *Host::GetD3DDevice() const
{
	return _d3dDevice;
}

ID3D11DeviceContext * Host::GetD3DImmediateContext() const
{
	return _d3dContext;
}

ID2D1Device *Host::GetD2DDevice() const
{
	return _d2dDevice;
}

IRenderSettingsManager* Host::GetRenderSettings() const 
{
	return ( IRenderSettingsManager * ) &_renderSettings;
}

IRenderTimer* Host::GetRenderTimer() const 
{
	return _timer;
}

bool Host::SetBackBufferRenderTarget(ID2D1DeviceContext *context)
{
	if ( !context ) return false;

	LOG( "Setting D2D device context render target to backbuffer...", LOG_HIGH );
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
			SAFE_RELEASE( dxgiSurface );
			return true;
		}
		SAFE_RELEASE( dxgiSurface );
	}
	return false;
}


void Host::FatalError( const char *sender, const char *message )
{
	std::lock_guard<std::mutex> lock( _mutex );

	if ( sender && message ) {
		std::ostringstream ss;
		ss << "FATAL ERROR: " << message;
		Logger::Instance().Add( sender, ss.str().c_str(), LOG_ERROR );
	}
	LOG( "Notified of fatal error, telling module to panic", LOG_ERROR );
	Logger::Instance().Flush();

	if ( _module != nullptr ) {
		_module->Panic();
	}

	_fatalErrorHandler( sender ? sender : "", message ? message : "" );  //signal any callbacks to the fatal error event

	TerminateProcess( GetCurrentProcess(), 1 );
}

const Version *Host::GetMGDFVersion() const
{
	return &_version;
}

ILogger *Host::GetLogger() const
{
	return &Logger::Instance();
}

ITimer * Host::GetTimer() const
{
	return _timer;
}

void Host::QueueShutDown()
{
	_shutdownQueued.store( true );
}

void Host::ShutDown()
{
	_shutDownHandler();//message the shutdown callback
}

MGDFError Host::Load( const char *saveName, wchar_t *loadBuffer, UINT32 *size, Version &version )
{
	if ( !saveName ) {
		LOG( "save name cannot be null. Only alphanumeric characters and the space character are permitted", LOG_ERROR );
		return MGDF_ERR_INVALID_SAVE_NAME;
	}

	LOG( "Loading from " << saveName << "...", LOG_LOW );
	std::string loadName( saveName );
	std::wstring loadDataDir = Resources::Instance().SaveDataDir( loadName );
	std::wstring loadFile = Resources::Instance().GameStateSaveFile( loadName );
	std::wstring loadDir = Resources::Instance().SaveDir( loadName );

	if ( loadDataDir.size() + 1 > UINT32_MAX ) {
		FATALERROR( this, "Unable to store load data path in 32bit buffer" );
		return MGDF_ERR_FATAL;
	}

	if ( loadDataDir.size() + 1 > *size || loadBuffer == nullptr ) {
		*size = static_cast<UINT32>( loadDataDir.size() ) + 1;
		return MGDF_ERR_BUFFER_TOO_SMALL;
	} else {
		*size = static_cast<UINT32>( loadDataDir.size() ) + 1;
		memcpy( loadBuffer, loadDataDir.c_str(), sizeof( wchar_t ) * ( *size ) );

		std::auto_ptr<storage::IGameStateStorageHandler> handler( _storage->CreateGameStateStorageHandler( _game->GetUid(), _game->GetVersion() ) );
		_ASSERTE( handler.get() );

		MGDFError error = handler->Load( loadFile );
		if ( MGDF_OK != error ) {
			FATALERROR( this, "Unable to load game state data from " + Resources::ToString( loadDir ) );
			return error;
		}

		version = *handler->GetVersion();
		return MGDF_OK;
	}
}

MGDFError Host::BeginSave( const char *save, wchar_t *saveBuffer, UINT32 *size )
{
	if ( !save ) {
		LOG( "save name cannot be null. Only alphanumeric characters and the space character are permitted", LOG_ERROR );
		return MGDF_ERR_INVALID_SAVE_NAME;
	}

	LOG( "Beginning save to " << save << "...", LOG_LOW );
	std::string saveName( save );

	//only alphanumerics and space allowed.
	for ( auto c : saveName ) {
		if ( !isalnum( c ) && c != ' ' ) {
			LOG( saveName << " is an invalid save name. Only alphanumeric characters and the space character are permitted", LOG_ERROR );
			return MGDF_ERR_INVALID_SAVE_NAME;
		}
	}

	saveName.insert( 0, PENDING_SAVE_PREFIX );

	std::wstring saveBufferContent( Resources::Instance().SaveDataDir( saveName ) );

	if ( saveBufferContent.size() + 1 > UINT32_MAX ) {
		FATALERROR( this, "Unable to store save data path in 32bit buffer" );
		return MGDF_ERR_FATAL;
	}

	if ( saveBufferContent.size() + 1 > *size  || saveBuffer == nullptr ) {
		*size = static_cast<UINT32>( saveBufferContent.size() ) + 1;
		return MGDF_ERR_BUFFER_TOO_SMALL;
	} else {
		*size = static_cast<UINT32>( saveBufferContent.size() ) + 1;
		memcpy( saveBuffer, saveBufferContent.c_str(), sizeof( wchar_t ) * ( *size ) );
	}

	try {
		//create the subdir for the names save files
		wpath saveDir( Resources::Instance().SaveDir( saveName ) );
		if ( !exists( saveDir ) )
			create_directory( saveDir );
		else {
			remove_all( saveDir );  //clear the dir
			create_directory( saveDir );  //recreate it
		}
		//create the save data sub-folder
		wpath saveDataDir( saveBufferContent );
		create_directory( saveDataDir );

		std::auto_ptr<storage::IGameStateStorageHandler> handler( _storage->CreateGameStateStorageHandler( _game->GetUid(), _game->GetVersion() ) );
		handler->Save( Resources::Instance().GameStateSaveFile( saveName ) );

		return MGDF_OK;
	} catch ( ... ) {
		FATALERROR( this, "Unable to load game state data from " << Resources::ToString( saveBufferContent ) );
		return MGDF_ERR_FATAL;
	}
}

MGDFError Host::CompleteSave( const char *save )
{
	if ( !save ) {	
		LOG( "save name cannot be null. Only alphanumeric characters and the space character are permitted", LOG_ERROR );
		return MGDF_ERR_INVALID_SAVE_NAME;
	}

	LOG( "Completing save to " << save << "...", LOG_LOW );
	try {
		std::string saveName( save );
		std::string pendingSave( save );
		pendingSave.insert( 0, PENDING_SAVE_PREFIX );

		wpath pendingSaveDir( Resources::Instance().SaveDir( pendingSave ) );
		wpath saveDir( Resources::Instance().SaveDir( saveName ) );

		if ( !exists( pendingSaveDir ) ) {
			LOG( saveName << " is not a pending save. Ensure that BeginSave is called with a matching save name before calling CompleteSave", LOG_ERROR );
			return MGDF_ERR_NO_PENDING_SAVE;
		}

		if ( exists( saveDir ) ) {
			remove_all( saveDir );
		}
		//swap the pending with the completed save
		rename( pendingSaveDir, saveDir );

		//update the list of save games
		GetSaves();
		bool exists = false;
		for ( auto save : *_saves->Items() ) {
			if ( strcmp( saveName.c_str(), save ) == 0 ) {
				exists = true;
				break;
			}
		}
		if ( !exists ) {
			char *copy = new char[saveName.size() + 1];
			strcpy_s( copy, saveName.size() + 1, saveName.c_str() );
			_saves->Add( copy );
		}

		return MGDF_OK;
	} catch ( ... ) {
		FATALERROR( this, "Unable to complete loading game state data" );
		return MGDF_ERR_FATAL;
	}
}

const IStringList *Host::GetSaves() const
{
	if ( _saves == nullptr ) {
		LOG( "Enumerating saves...", LOG_MEDIUM );
		const_cast<StringList *>( _saves ) = new StringList();

		wpath savePath( Resources::Instance().SaveBaseDir() );

		wdirectory_iterator end_itr; // default construction yields past-the-end
		for ( wdirectory_iterator itr( savePath ); itr != end_itr; ++itr ) {
			if ( is_directory( itr->path() ) ) {
				std::string saveName( Resources::ToString( itr->path().filename() ) );
				if ( saveName.find( PENDING_SAVE_PREFIX ) != 0 ) {
					char *copy = new char[saveName.size() + 1];
					strcpy_s( copy, saveName.size() + 1, saveName.c_str() );
					_saves->Add( copy );  //add the save folder to the list
				}
			}
		}
	}
	return _saves;
}

void Host::RemoveSave( const char *saveName )
{
	if ( !saveName ) return;

	GetSaves();

	for ( UINT32 i = 0; i < _saves->Size(); ++i ) {
		if ( strcmp( saveName, _saves->Get( i ) ) == 0 ) {
			LOG( "Removing save " << saveName << "...", LOG_MEDIUM );
			delete[] _saves->Get( i );
			_saves->Remove( i );
			wpath savePath( Resources::Instance().UserBaseDir() + Resources::ToWString( saveName ) );
			remove_all( savePath );
			return;
		}
	}
}

IGame *Host::GetGame() const
{
	return _game;
}

IStatisticsManager *Host::GetStatistics() const
{
	return _stats;
}

IVirtualFileSystem *Host::GetVFS() const
{
	return _vfs;
}

IInputManager *Host::GetInput() const
{
	return _input;
}

ISoundManager *Host::GetSound() const
{
	return _sound;
}

void Host::ClearWorkingDirectory()
{
	LOG( "Clearing working directory...", LOG_HIGH );
	wpath workingDir( Resources::Instance().WorkingDir() );
	if ( exists( workingDir ) ) {
		remove_all( workingDir );
	} else {
		create_directory( workingDir );
	}
}

const char* Host::GetErrorDescription( MGDFError err ) const
{
	static std::string ok( "No error" );
	static std::string allocatingBuffer( "Error allocating audio buffer" );
	static std::string noFreeSources( "No free audio sources" );
	static std::string vorbisFailed( "Failed to load vorbis audio library" );
	static std::string invalidFormat( "Invalid audio file format" );
	static std::string invalidArchive( "Invalid archive file" );
	static std::string invalidFile( "Not a valid VFS file" );
	static std::string noPending( "This save name does not match any pending save created by BeginSave" );
	static std::string invalidSave( "Invalid save name - only alphanumeric characters and the space character are permitted" );
	static std::string archiveToLarge( "Archive file is too large - archive files cannot be over 4GB in size" );
	static std::string fileInUse( "File is already open for reading elsewhere" );
	static std::string bufferTooSmall( "Target buffer is too small to hold all required data" );
	static std::string fatal( "Fatal error - shutting down" );
	static std::string folder( "Cannot open a folder for reading" );
	static std::string gpuTimer( "GPU Timers are not supported at this DirectX feature level, only 10 and up is supported" );
	static std::string invalidTimerName( "Timer names cannot be null" );
	static std::string invalidJson( "Invalid JSON data" );
	static std::string invalidParam( "Invalid parameter format" );
	static std::string audioInitFailed( "Failed to initialize audio system" );
	static std::string cpuTimer( "High resolution timers are unsupported on this system" );
	static std::string unknown( "Unknown error" );

	switch ( err )
	{
	case MGDF_OK:
		return ok.c_str();
	case MGDF_ERR_ERROR_ALLOCATING_BUFFER:
		return allocatingBuffer.c_str();
	case MGDF_ERR_NO_FREE_SOURCES:
		return noFreeSources.c_str();
	case MGDF_ERR_VORBIS_LIB_LOAD_FAILED:
		return vorbisFailed.c_str();
	case MGDF_ERR_INVALID_FORMAT:
		return invalidFormat.c_str();
	case MGDF_ERR_INVALID_ARCHIVE_FILE:
		return invalidArchive.c_str();
	case MGDF_ERR_INVALID_FILE:
		return invalidFile.c_str();
	case MGDF_ERR_NO_PENDING_SAVE:
		return noPending.c_str();
	case MGDF_ERR_INVALID_SAVE_NAME:
		return invalidSave.c_str();
	case MGDF_ERR_ARCHIVE_FILE_TOO_LARGE:
		return archiveToLarge.c_str();
	case MGDF_ERR_FILE_IN_USE:
		return fileInUse.c_str();
	case MGDF_ERR_BUFFER_TOO_SMALL:
		return bufferTooSmall.c_str();
	case MGDF_ERR_FATAL:
		return fatal.c_str();
	case MGDF_ERR_IS_FOLDER:
		return folder.c_str();
	case MGDF_ERR_GPU_TIMER_UNSUPPORTED:
		return gpuTimer.c_str();
	case MGDF_ERR_INVALID_TIMER_NAME:
		return invalidTimerName.c_str();
	case MGDF_ERR_INVALID_JSON:
		return invalidJson.c_str();
	case MGDF_ERR_INVALID_PARAMETER:
		return invalidParam.c_str();
	case MGDF_ERR_AUDIO_INIT_FAILED:
		return audioInitFailed.c_str();
	case MGDF_ERR_CPU_TIMER_UNSUPPORTED:
		return cpuTimer.c_str();
	default:
		return unknown.c_str();
	}
}

const char* Host::GetErrorString( MGDFError err ) const
{
	static std::string ok( "MGDF_OK" );
	static std::string allocatingBuffer( "MGDF_ERR_ERROR_ALLOCATING_BUFFER" );
	static std::string noFreeSources( "MGDF_ERR_NO_FREE_SOURCES" );
	static std::string vorbisFailed( "MGDF_ERR_VORBIS_LIB_LOAD_FAILED" );
	static std::string invalidFormat( "MGDF_ERR_INVALID_FORMAT" );
	static std::string invalidArchive( "MGDF_ERR_INVALID_ARCHIVE_FILE" );
	static std::string invalidFile( "MGDF_ERR_INVALID_FILE" );
	static std::string noPending( "MGDF_ERR_NO_PENDING_SAVE" );
	static std::string invalidSave( "MGDF_ERR_INVALID_SAVE_NAME" );
	static std::string archiveToLarge( "MGDF_ERR_ARCHIVE_FILE_TOO_LARGE" );
	static std::string fileInUse( "MGDF_ERR_FILE_IN_USE" );
	static std::string bufferTooSmall( "MGDF_ERR_BUFFER_TOO_SMALL" );
	static std::string fatal( "MGDF_ERR_FATAL" );
	static std::string folder( "MGDF_ERR_IS_FOLDER" );
	static std::string gpuTimer( "MGDF_ERR_GPU_TIMER_UNSUPPORTED" );
	static std::string invalidTimerName( "MGDF_ERR_INVALID_TIMER_NAME" );
	static std::string invalidJson( "MGDF_ERR_INVALID_JSON" );
	static std::string invalidParam( "MGDF_ERR_INVALID_PARAMETER" );
	static std::string audioInitFailed( "MGDF_ERR_AUDIO_INIT_FAILED" );
	static std::string cpuTimer( "MGDF_ERR_CPU_TIMER_UNSUPPORTED" );
	static std::string unknown( "MGDF_ERR_UNKNOWN" );

	switch ( err )
	{
	case MGDF_OK:
		return ok.c_str();
	case MGDF_ERR_ERROR_ALLOCATING_BUFFER:
		return allocatingBuffer.c_str();
	case MGDF_ERR_NO_FREE_SOURCES:
		return noFreeSources.c_str();
	case MGDF_ERR_VORBIS_LIB_LOAD_FAILED:
		return vorbisFailed.c_str();
	case MGDF_ERR_INVALID_FORMAT:
		return invalidFormat.c_str();
	case MGDF_ERR_INVALID_ARCHIVE_FILE:
		return invalidArchive.c_str();
	case MGDF_ERR_INVALID_FILE:
		return invalidFile.c_str();
	case MGDF_ERR_NO_PENDING_SAVE:
		return noPending.c_str();
	case MGDF_ERR_INVALID_SAVE_NAME:
		return invalidSave.c_str();
	case MGDF_ERR_ARCHIVE_FILE_TOO_LARGE:
		return archiveToLarge.c_str();
	case MGDF_ERR_FILE_IN_USE:
		return fileInUse.c_str();
	case MGDF_ERR_BUFFER_TOO_SMALL:
		return bufferTooSmall.c_str();
	case MGDF_ERR_FATAL:
		return fatal.c_str();
	case MGDF_ERR_IS_FOLDER:
		return folder.c_str();
	case MGDF_ERR_GPU_TIMER_UNSUPPORTED:
		return gpuTimer.c_str();
	case MGDF_ERR_INVALID_TIMER_NAME:
		return invalidTimerName.c_str();
	case MGDF_ERR_INVALID_JSON:
		return invalidJson.c_str();
	case MGDF_ERR_INVALID_PARAMETER:
		return invalidParam.c_str();
	case MGDF_ERR_AUDIO_INIT_FAILED:
		return audioInitFailed.c_str();
	case MGDF_ERR_CPU_TIMER_UNSUPPORTED:
		return cpuTimer.c_str();
	default:
		return unknown.c_str();
	}
}

}
}