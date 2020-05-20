#pragma once

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFHost.hpp>
#include <MGDF/MGDFModule.hpp>
#include <atomic>
#include <functional>
#include <mutex>
#include <sstream>

#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../common/MGDFListImpl.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFSystemComponent.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFDebugImpl.hpp"
#include "MGDFGameImpl.hpp"
#include "MGDFHostStats.hpp"
#include "MGDFModuleFactory.hpp"
#include "MGDFRenderSettingsManagerImpl.hpp"
#include "MGDFStatisticsManagerImpl.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

/**
this class represents a slightly more functional version of the host interface
with a number of internal implementation additions, this class should be
inherited to create mock host objects
*/
class IHostImpl : public IRenderHost, public ISimHost {
 public:
  virtual ~IHostImpl(void){};

  // handler callbacks
  typedef std::function<void(void)>
      ShutDownFunction;  // shutDown callback function signature
  typedef std::function<void(const std::string &, const std::string &)>
      FatalErrorFunction;  // fatal error callback function signature

  void SetShutDownHandler(const ShutDownFunction handler);
  void SetFatalErrorHandler(const FatalErrorFunction handler);

 protected:
  // event callbacks
  ShutDownFunction _shutDownHandler;
  FatalErrorFunction _fatalErrorHandler;
};

typedef ListImpl<IStringList, const char *> StringList;

struct HostComponents {
  storage::IStorageFactoryComponent *Storage;
  ComObject<input::IInputManagerComponent> Input;
  ComObject<audio::ISoundManagerComponent> Sound;
  ComObject<vfs::IVirtualFileSystemComponent> VFS;
};

/**
 reference implementation of the Host interfaces
\author gcconner
*/
class Host : public IHostImpl {
 public:
  static MGDFError TryCreate(ComObject<Game> game, HostComponents &components,
                             Host **host);

  virtual ~Host(void);

  void STCreateModule();
  void STUpdate(double simulationTime, HostStats &stats);
  void STDisposeModule();

  void RTBeforeFirstDraw();
  void RTSetDevices(HWND window, ID3D11Device *device, ID2D1Device *d2dDevice,
                    IDXGIAdapter1 *adapter);
  void RTDraw(double alpha);
  void RTBeforeBackBufferChange();
  void RTBackBufferChange(ID3D11Texture2D *backBuffer,
                          ID3D11Texture2D *depthStencilBuffer);
  void RTBeforeDeviceReset();
  void RTDeviceReset();

  UINT32 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT32 *featureLevelsSize);
  RenderSettingsManager &GetRenderSettingsImpl();
  ComObject<input::IInputManagerComponent> GetInputManagerImpl();
  ComObject<Debug> GetDebugImpl();

  // error handling functions
  void FatalError(const char *, const char *) override final;

  // ICommonHost methods
  ILogger *GetLogger() const override final;
  IRenderSettingsManager *GetRenderSettings() const override final;
  ITimer *GetTimer() const override final;
  const Version *GetMGDFVersion() const override final;
  const char *GetErrorDescription(MGDFError err) const override final;
  const char *GetErrorString(MGDFError err) const override final;
  void GetDebug(IDebug **debug) override final;

  // ISimHost methods
  void QueueShutDown() override final;
  MGDFError Load(const char *saveName, wchar_t *loadBuffer, UINT32 *size,
                 Version *version) override final;
  MGDFError BeginSave(const char *saveName, wchar_t *saveBuffer,
                      UINT32 *size) override final;
  MGDFError CompleteSave(const char *saveName) override final;
  void GetVFS(IVirtualFileSystem **vfs) override final;
  void GetSound(ISoundManager **sound) override final;
  IStatisticsManager *GetStatistics() const override final;
  void GetGame(IGame **game) override final;
  void GetInput(IInputManager **manager) override final;
  void ShutDown() override final;
  const IStringList *GetSaves() const override final;
  void RemoveSave(const char *saveName) override final;

  // IRenderHost methods
  ID3D11Device *GetD3DDevice() const override final;
  ID3D11DeviceContext *GetD3DImmediateContext() const override final;
  ID2D1Device *GetD2DDevice() const override final;
  IRenderTimer *GetRenderTimer() const override final;
  bool SetBackBufferRenderTarget(ID2D1DeviceContext *context) override final;
  ID3D11Texture2D *GetBackBuffer() const override final;
  ID3D11Texture2D *GetDepthStencilBuffer() const override final;
  void GetBackBufferDescription(
      D3D11_TEXTURE2D_DESC *backBufferDesc,
      D3D11_TEXTURE2D_DESC *depthStencilBufferDesc) const override final;

 private:
  Host(ComObject<Game> game, HostComponents &components);
  MGDFError Init();

  void ClearWorkingDirectory();

  IModule *_module;  // the currently executing module
  ModuleFactory *_moduleFactory;

  storage::IStorageFactoryComponent *_storage;
  ComObject<input::IInputManagerComponent> _input;
  ComObject<audio::ISoundManagerComponent> _sound;
  ComObject<vfs::IVirtualFileSystemComponent> _vfs;
  ComObject<Debug> _debugOverlay;
  ComObject<Game> _game;
  StringList *_saves;
  RenderSettingsManager _renderSettings;
  StatisticsManager *_stats;

  ID3D11Device *_d3dDevice;
  ID3D11DeviceContext *_d3dContext;
  ID2D1Device *_d2dDevice;
  ID3D11Texture2D *_backBuffer;
  ID3D11Texture2D *_depthStencilBuffer;

  std::mutex _mutex;
  Version _version;
  Timer *_timer;
  std::atomic<bool> _shutdownQueued;
  mutable std::atomic<bool> _showDebug;
};

}  // namespace core
}  // namespace MGDF