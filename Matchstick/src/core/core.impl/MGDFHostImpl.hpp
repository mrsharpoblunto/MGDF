#pragma once

#include <atomic>
#include <functional>
#include <sstream>
#include <boost/thread/thread.hpp>

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
class IHostImpl: public IRenderHost, public ISimHost, public IErrorHandler
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
	Host( Game *game );

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
	void FatalError( const char *, const char * ) override;

	// ICommonHost methods
	ILogger *GetLogger() const override;
	IRenderSettingsManager *GetRenderSettings() const override;
	ITimer * GetTimer() const override;
	const Version * GetMGDFVersion() const override;

	// ISimHost methods
	void QueueShutDown() override;
	MGDFError Load( const char *saveName, wchar_t *loadBuffer, UINT32 *size, Version &version ) override;
	MGDFError BeginSave( const char *saveName, wchar_t *saveBuffer, UINT32 *size ) override;
	MGDFError CompleteSave( const char *saveName ) override;
	IVirtualFileSystem *GetVFS() const override;
	ISoundManager *GetSound() const override;
	IStatisticsManager *GetStatistics() const override;
	IGame *GetGame() const override;
	IInputManager *GetInput() const override;
	void ShutDown() override;
	const IStringList *GetSaves() const override;
	void RemoveSave( const char *saveName ) override;

	// IRenderHost methods
	ID3D11Device * GetD3DDevice() const override;
	ID3D11DeviceContext * GetD3DImmediateContext() const override;
	ID2D1Device * GetD2DDevice() const override;
	IRenderTimer * GetRenderTimer() const override;
	bool SetBackBufferRenderTarget( ID2D1DeviceContext *context ) override;
	ID3D11Texture2D * GetBackBuffer() const override;
	void GetBackBufferDescription( D3D11_TEXTURE2D_DESC *desc ) const override;

private:
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

	boost::mutex _mutex;
	Version _version;
	Timer _timer;
	std::atomic<bool> _shutdownQueued;
};

}
}