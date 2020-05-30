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

typedef ListImpl<IStringList, const char *> StringList;

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
  ComObject<RenderSettingsManager> GetRenderSettingsImpl();
  ComObject<input::IInputManagerComponent> GetInputManagerImpl();
  ComObject<Debug> GetDebugImpl();

  // IUnknown methods
  ULONG AddRef() override;
  ULONG Release() override;
  HRESULT QueryInterface(REFIID riid, void **ppvObject);

  // ILogger methods
  void SetLoggingLevel(LogLevel level) override final;
  LogLevel GetLoggingLevel() const override final;
  void Log(const char *sender, const char *message,
           LogLevel level) override final;

  // ICommonHost methods
  void FatalError(const char *, const char *) override final;
  void GetRenderSettings(IRenderSettingsManager **settings) override final;
  void GetTimer(ITimer **timer) override final;
  const Version *GetMGDFVersion() const override final;
  const char *GetErrorDescription(MGDFError err) const override final;
  const char *GetErrorString(MGDFError err) const override final;
  void GetDebug(IDebug **debug) override final;

  // ISimHost methods
  void QueueShutDown() override final;
  void GetSaves(ISaveManager **saves) override final;
  void GetVFS(IVirtualFileSystem **vfs) override final;
  void GetSound(ISoundManager **sound) override final;
  void GetStatistics(IStatisticsManager **statistics) override final;
  void GetGame(IGame **game) override final;
  void GetInput(IInputManager **manager) override final;
  void ShutDown() override final;

  // IRenderHost methods
  ID3D11Device *GetD3DDevice() const override final;
  ID3D11DeviceContext *GetD3DImmediateContext() const override final;
  ID2D1Device *GetD2DDevice() const override final;
  bool SetBackBufferRenderTarget(ID2D1DeviceContext *context) override final;
  ID3D11Texture2D *GetBackBuffer() const override final;
  ID3D11Texture2D *GetDepthStencilBuffer() const override final;
  void GetBackBufferDescription(
      D3D11_TEXTURE2D_DESC *backBufferDesc,
      D3D11_TEXTURE2D_DESC *depthStencilBufferDesc) const override final;

  HRESULT CreateCPUCounter(const char *name,
                           IPerformanceCounter **counter) override final;
  HRESULT CreateGPUCounter(const char *name,
                           IPerformanceCounter **counter) override final;

 private:
  Host(ComObject<Game> game, HostComponents &components);
  HRESULT Init();

  void ClearWorkingDirectory();
  void EnumerateSaves(
      std::function<bool(const std::filesystem::path &path)> handler) const;

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

  ID3D11Device *_d3dDevice;
  ID3D11DeviceContext *_d3dContext;
  ID2D1Device *_d2dDevice;
  ID3D11Texture2D *_backBuffer;
  ID3D11Texture2D *_depthStencilBuffer;

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