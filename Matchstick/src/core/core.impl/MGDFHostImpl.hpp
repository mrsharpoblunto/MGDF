#pragma once

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFHost.hpp>
#include <MGDF/MGDFModule.hpp>
#include <atomic>
#include <functional>
#include <mutex>
#include <sstream>

#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFDebugImpl.hpp"
#include "MGDFGameImpl.hpp"
#include "MGDFHostStats.hpp"
#include "MGDFModuleFactory.hpp"
#include "MGDFRenderSettingsManagerImpl.hpp"
#include "MGDFSaveManagerImpl.hpp"
#include "MGDFStatisticsManagerImpl.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

struct HostComponents {
  std::shared_ptr<storage::IStorageFactoryComponent> Storage;
  ComObject<input::IInputManagerComponent> Input;
  ComObject<audio::ISoundManagerComponent> Sound;
  ComObject<vfs::IVirtualFileSystemComponent> VFS;
};

/**
 reference implementation of the Host interfaces
\author gcconner
*/
class Host : public IRenderHost, public ISimHost {
 public:
  static HRESULT TryCreate(ComObject<Game> game, HostComponents &components,
                           ComObject<Host> &host);

  Host(ComObject<Game> game, HostComponents &components);
  virtual ~Host(void);

  // handler callbacks
  typedef std::function<void(void)>
      ShutDownFunction;  // shutDown callback function signature
  typedef std::function<void(const std::string &, const std::string &)>
      FatalErrorFunction;  // fatal error callback function signature

  void SetShutDownHandler(const ShutDownFunction handler);
  void SetFatalErrorHandler(const FatalErrorFunction handler);

  void STCreateModule();
  void STUpdate(double simulationTime, HostStats &stats);
  void STDisposeModule();

  void RTBeforeFirstDraw();
  void RTSetDevices(HWND window, const ComObject<ID3D11Device> &device,
                    const ComObject<ID2D1Device> &d2dDevice,
                    const ComObject<IDXGIAdapter1> &adapter);
  void RTDraw(double alpha);
  void RTBeforeBackBufferChange();
  void RTBackBufferChange(const ComObject<ID3D11Texture2D> &backBuffer,
                          const ComObject<ID3D11Texture2D> &depthStencilBuffer);
  void RTBeforeDeviceReset();

  UINT32 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT32 *featureLevelsSize);
  ComObject<RenderSettingsManager> GetRenderSettingsImpl();
  ComObject<input::IInputManagerComponent> GetInputManagerImpl();
  ComObject<Debug> GetDebugImpl();

  // IUnknown methods
  ULONG AddRef() final;
  ULONG Release() final;
  HRESULT QueryInterface(REFIID riid, void **ppvObject) final;

  // ILogger methods
  void SetLoggingLevel(LogLevel level) final;
  LogLevel GetLoggingLevel() const final;
  void Log(const char *sender, const char *message, LogLevel level) final;

  // ICommonHost methods
  void FatalError(const char *, const char *) final;
  void GetRenderSettings(IRenderSettingsManager **settings) final;
  void GetTimer(ITimer **timer) final;
  const Version *GetMGDFVersion() const final;
  const char *GetErrorDescription(MGDFError err) const final;
  const char *GetErrorString(MGDFError err) const final;
  void GetDebug(IDebug **debug) final;

  // ISimHost methods
  void QueueShutDown() final;
  void GetSaves(ISaveManager **saves) final;
  void GetVFS(IVirtualFileSystem **vfs) final;
  void GetSound(ISoundManager **sound) final;
  void GetStatistics(IStatisticsManager **statistics) final;
  void GetGame(IGame **game) final;
  void GetInput(IInputManager **manager) final;
  void ShutDown() final;

  // IRenderHost methods
  ID3D11Device *GetD3DDevice() const final;
  ID3D11DeviceContext *GetD3DImmediateContext() const final;
  ID2D1Device *GetD2DDevice() const final;
  bool SetBackBufferRenderTarget(ID2D1DeviceContext *context) final;
  ID3D11Texture2D *GetBackBuffer() const final;
  ID3D11Texture2D *GetDepthStencilBuffer() const final;
  void GetBackBufferDescription(
      D3D11_TEXTURE2D_DESC *backBufferDesc,
      D3D11_TEXTURE2D_DESC *depthStencilBufferDesc) const final;

  HRESULT CreateCPUCounter(const char *name,
                           IPerformanceCounter **counter) final;
  HRESULT CreateGPUCounter(const char *name,
                           IPerformanceCounter **counter) final;

 private:
  HRESULT Init();

  void ClearWorkingDirectory();

  ComObject<IModule> _module;  // the currently executing module
  std::unique_ptr<ModuleFactory> _moduleFactory;

  std::shared_ptr<storage::IStorageFactoryComponent> _storage;
  ComObject<SaveManager> _saves;
  ComObject<input::IInputManagerComponent> _input;
  ComObject<audio::ISoundManagerComponent> _sound;
  ComObject<vfs::IVirtualFileSystemComponent> _vfs;
  ComObject<Debug> _debugOverlay;
  ComObject<Game> _game;
  ComObject<Timer> _timer;
  ComObject<RenderSettingsManager> _renderSettings;
  ComObject<StatisticsManager> _stats;

  ComObject<ID3D11Device> _d3dDevice;
  ComObject<ID3D11DeviceContext> _d3dContext;
  ComObject<ID2D1Device> _d2dDevice;
  ComObject<ID3D11Texture2D> _backBuffer;
  ComObject<ID3D11Texture2D> _depthStencilBuffer;

  std::mutex _mutex;
  Version _version;
  std::atomic<bool> _shutdownQueued;
  std::atomic<ULONG> _references;
  mutable std::atomic<bool> _showDebug;

  // event callbacks
  ShutDownFunction _shutDownHandler;
  FatalErrorFunction _fatalErrorHandler;
};

}  // namespace core
}  // namespace MGDF