#pragma once

#include <atomic>
#include <functional>
#include <sstream>
#include <mutex>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFHost.hpp>
#include <MGDF/MGDFModule.hpp>

#include "../common/MGDFListImpl.hpp"
#include "../common/MGDFSystemComponent.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"

#include "MGDFGameImpl.hpp"
#include "MGDFStatisticsManagerImpl.hpp"
#include "MGDFRenderSettingsManagerImpl.hpp"
#include "MGDFModuleFactory.hpp"
#include "MGDFHostStats.hpp"
#include "MGDFTimer.hpp"

namespace MGDF
{
namespace core
{

/**
this class represents a slightly more functional version of the host interface with a number of internal
implementation additions, this class should be inherited to create mock host objects
*/
class IHostImpl: public IRenderHost, public ISimHost
{
public:
	virtual ~IHostImpl( void ) {};

	// handler callbacks
	typedef std::function<void (void) > ShutDownFunction; //shutDown callback function signature
	typedef std::function<void ( const std::string &, const std::string &) > FatalErrorFunction; //fatal error callback function signature

	void SetShutDownHandler( const ShutDownFunction handler );
	void SetFatalErrorHandler( const FatalErrorFunction handler );
protected:
	//event callbacks
	ShutDownFunction _shutDownHandler;
	FatalErrorFunction _fatalErrorHandler;
};

typedef ListImpl<IStringList, const char *> StringList;

/**
 reference implementation of the Host interfaces
\author gcconner
*/
class Host: public IHostImpl
{
public:
	static MGDFError TryCreate( Game *game, Host **host );

	virtual ~Host( void );

	void STCreateModule();
	void STUpdate( double simulationTime, HostStats &stats );
	void STDisposeModule();

	void RTBeforeFirstDraw();
	void RTSetDevices( ID3D11Device *device, ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter );
	void RTDraw( double alpha );
	void RTBeforeBackBufferChange();
	void RTBackBufferChange( ID3D11Texture2D *backBuffer );
	void RTBeforeDeviceReset();
	void RTDeviceReset();

	UINT32 GetCompatibleD3DFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *featureLevelsSize );
	RenderSettingsManager &GetRenderSettingsImpl();
	input::IInputManagerComponent &GetInputManagerImpl() const;
	void GetHostInfo( const HostStats &stats, std::wstringstream &ss ) const;

	//error handling functions
	void FatalError( const char *, const char * ) override final;

	// ICommonHost methods
	ILogger *GetLogger() const override final;
	IRenderSettingsManager *GetRenderSettings() const override final;
	ITimer * GetTimer() const override final;
	const Version * GetMGDFVersion() const override final;
	const char* GetErrorDescription( MGDFError err ) const override final;
	const char* GetErrorString( MGDFError err ) const override final;

	// ISimHost methods
	void QueueShutDown() override final;
	MGDFError Load( const char *saveName, wchar_t *loadBuffer, UINT32 *size, Version &version ) override final;
	MGDFError BeginSave( const char *saveName, wchar_t *saveBuffer, UINT32 *size ) override final;
	MGDFError CompleteSave( const char *saveName ) override final;
	IVirtualFileSystem *GetVFS() const override final;
	ISoundManager *GetSound() const override final;
	IStatisticsManager *GetStatistics() const override final;
	IGame *GetGame() const override final;
	IInputManager *GetInput() const override final;
	void ShutDown() override final;
	const IStringList *GetSaves() const override final;
	void RemoveSave( const char *saveName ) override final;

	// IRenderHost methods
	ID3D11Device * GetD3DDevice() const override final;
	ID3D11DeviceContext * GetD3DImmediateContext() const override final;
	ID2D1Device * GetD2DDevice() const override final;
	IRenderTimer * GetRenderTimer() const override final;
	bool SetBackBufferRenderTarget( ID2D1DeviceContext *context ) override final;
	ID3D11Texture2D * GetBackBuffer() const override final;
	void GetBackBufferDescription( D3D11_TEXTURE2D_DESC *desc ) const override final;

private:
	Host( Game *game );
	MGDFError Init();

	void ClearWorkingDirectory();

	IModule * _module; //the currently executing module
	ModuleFactory *_moduleFactory;

	storage::IStorageFactoryComponent *_storage;
	input::IInputManagerComponent *_input;
	audio::ISoundManagerComponent *_sound;
	vfs::IVirtualFileSystemComponent *_vfs;
	Game *_game;
	StringList *_saves;
	RenderSettingsManager _renderSettings;
	StatisticsManager *_stats;

	ID3D11Device *_d3dDevice;
	ID3D11DeviceContext *_d3dContext;
	ID2D1Device *_d2dDevice;
	ID3D11Texture2D *_backBuffer;

	std::mutex _mutex;
	Version _version;
	Timer *_timer;
	std::atomic<bool> _shutdownQueued;
};

}
}