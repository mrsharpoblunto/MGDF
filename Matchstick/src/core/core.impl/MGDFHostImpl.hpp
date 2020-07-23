#pragma once

#include <MGDF/MGDF.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <sstream>

#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../vfs/MGDFReadOnlyVirtualFileSystemComponentImpl.hpp"
#include "../vfs/MGDFWriteableVirtualFileSystem.hpp"
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
  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> VFS;
};

/**
 reference implementation of the Host interfaces
\author gcconner
*/
class Host : public IMGDFRenderHost, public IMGDFSimHost {
 public:
  static HRESULT TryCreate(ComObject<Game> game, HostComponents &components,
                           ComObject<Host> &host);

  Host(ComObject<Game> game, HostComponents &components);
  virtual ~Host(void);

  // handler callbacks
  typedef std::function<void(void)>
      ShutDownFunction;  // shutDown callback function signature
  typedef std::function<void(void)>
      DeviceResetFunction;  // shutDown callback function signature
  typedef std::function<void(const std::string &, const std::string &)>
      FatalErrorFunction;  // fatal error callback function signature

  void SetShutDownHandler(const ShutDownFunction handler);
  void SetDeviceResetHandler(const DeviceResetFunction handler);
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
  void RTDeviceReset();

  UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT64 *featureLevelsSize);
  ComObject<RenderSettingsManager> GetRenderSettingsImpl();
  ComObject<input::IInputManagerComponent> GetInputManagerImpl();
  ComObject<Debug> GetDebugImpl();

  // IUnknown methods
  ULONG AddRef() final;
  ULONG Release() final;
  HRESULT QueryInterface(REFIID riid, void **ppvObject) final;

  // ILogger methods
  void _stdcall SetLoggingLevel(MGDFLogLevel level) final;
  MGDFLogLevel __stdcall GetLoggingLevel() final;
  void __stdcall Log(const char *sender, const char *message,
                     MGDFLogLevel level) final;

  // ICommonHost methods
  void __stdcall FatalError(const char *, const char *) final;
  void __stdcall GetRenderSettings(IMGDFRenderSettingsManager **settings) final;
  void __stdcall GetTimer(IMGDFTimer **timer) final;
  const MGDFVersion *__stdcall GetMGDFVersion() final;
  void _stdcall GetDebug(IMGDFDebug **debug) final;
  void __stdcall GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) final;
  void __stdcall GetWorkingVFS(IMGDFWriteableVirtualFileSystem **vfs) final;
  void __stdcall QueueDeviceReset() final;

  // ISimHost methods
  void __stdcall QueueShutDown() final;
  void __stdcall GetSaves(IMGDFSaveManager **saves) final;
  void __stdcall GetSound(IMGDFSoundManager **sound) final;
  void __stdcall GetStatistics(IMGDFStatisticsManager **statistics) final;
  void __stdcall GetGame(IMGDFGame **game) final;
  void __stdcall GetInput(IMGDFInputManager **manager) final;
  void __stdcall ShutDown() final;

  // IRenderHost methods
  void __stdcall GetD3DDevice(ID3D11Device **device) final;
  void __stdcall GetD2DDevice(ID2D1Device **device) final;
  BOOL __stdcall SetBackBufferRenderTarget(ID2D1DeviceContext *context) final;
  void __stdcall GetBackBuffer(ID3D11Texture2D **backbuffer) final;
  void __stdcall GetDepthStencilBuffer(ID3D11Texture2D **depthStencil) final;
  void __stdcall GetBackBufferDescription(
      D3D11_TEXTURE2D_DESC *backBufferDesc,
      D3D11_TEXTURE2D_DESC *depthStencilBufferDesc) final;

  HRESULT __stdcall CreateCPUCounter(const char *name,
                                     IMGDFPerformanceCounter **counter) final;
  HRESULT __stdcall CreateGPUCounter(const char *name,
                                     IMGDFPerformanceCounter **counter) final;

 private:
  HRESULT Init();

  void ClearWorkingDirectory();

  ComObject<IMGDFModule> _module;  // the currently executing module
  std::unique_ptr<ModuleFactory> _moduleFactory;

  std::shared_ptr<storage::IStorageFactoryComponent> _storage;
  ComObject<SaveManager> _saves;
  ComObject<input::IInputManagerComponent> _input;
  ComObject<audio::ISoundManagerComponent> _sound;
  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> _vfs;
  ComObject<vfs::WriteableVirtualFileSystem> _workingVfs;
  ComObject<Debug> _debugOverlay;
  ComObject<Game> _game;
  ComObject<Timer> _timer;
  ComObject<RenderSettingsManager> _renderSettings;
  ComObject<StatisticsManager> _stats;

  ComObject<ID3D11Device> _d3dDevice;
  ComObject<ID2D1Device> _d2dDevice;
  ComObject<ID3D11Texture2D> _backBuffer;
  ComObject<ID3D11Texture2D> _depthStencilBuffer;

  std::mutex _mutex;
  MGDFVersion _version;
  std::atomic<bool> _shutdownQueued;
  std::atomic<ULONG> _references;
  mutable std::atomic<bool> _showDebug;

  // event callbacks
  ShutDownFunction _shutDownHandler;
  DeviceResetFunction _deviceResetHandler;
  FatalErrorFunction _fatalErrorHandler;
};

}  // namespace core
}  // namespace MGDF