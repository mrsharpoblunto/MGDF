#include "StdAfx.h"

#include "MGDFHostImpl.hpp"

#include <filesystem>
#include <iomanip>

#include "../common/MGDFPreferenceConstants.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../vfs/archive/zip/ZipArchiveHandlerImpl.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"
#include "MGDFMetrics.hpp"
#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

namespace MGDF {
namespace core {

void Host::SetShutDownHandler(const ShutDownFunction handler) {
  _shutDownHandler = handler;
}

void Host::SetFatalErrorHandler(const FatalErrorFunction handler) {
  _fatalErrorHandler = handler;
}

void Host::SetDeviceResetHandler(const DeviceResetFunction handler) {
  _deviceResetHandler = handler;
}

HRESULT Host::TryCreate(ComObject<Game> game, HostComponents &components,
                        ComObject<Host> &host) {
  host = MakeCom<Host>(game, components);

  const HRESULT result = host->Init();
  if (FAILED(result)) {
    host.Clear();
  }
  return result;
}

Host::Host(ComObject<Game> game, HostComponents &components)
    : _game(game),
      _debugOverlay(nullptr),
      _module(nullptr),
      _version(VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION())),
      _storage(components.Storage),
      _input(components.Input),
      _sound(components.Sound),
      _vfs(components.VFS),
      _stats(MakeCom<StatisticsManager>(game->GetUid())),
      _renderSettings(MakeCom<RenderSettingsManager>()),
      _saves(MakeCom<SaveManager>(game, components.Storage)),
      _references(1UL),
      _d3dDevice(nullptr),
      _d2dDevice(nullptr),
      _depthStencilBuffer(nullptr),
      _backBuffer(nullptr) {
  _shutdownQueued.store(false);
  _ASSERTE(game);

  if (ParameterManager::Instance().HasParameter(
          ParameterConstants::STATISTICS_ENDPOINT_OVERRIDE)) {
    _stats->SetRemoteEndpoint(ParameterManager::Instance().GetParameter(
        ParameterConstants::STATISTICS_ENDPOINT_OVERRIDE));
  } else if (ParameterManager::Instance().HasParameter(
                 ParameterConstants::STATISTICS_ENABLED)) {
    _stats->SetRemoteEndpoint(_game->GetStatististicsService());
  }
}

ULONG Host::AddRef() { return ++_references; };

ULONG Host::Release() {
  const ULONG refs = --_references;
  if (refs == 0UL) {
    delete this;
  };
  return refs;
}
HRESULT Host::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (riid == IID_IUnknown || riid == __uuidof(IMGDFLogger) ||
      riid == __uuidof(IMGDFSimHost) || riid == __uuidof(IMGDFRenderHost) ||
      riid == __uuidof(IMGDFCommonHost)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
};

HRESULT Host::Init() {
  LOG("Creating Module factory...", MGDF_LOG_LOW);
  HRESULT result = ModuleFactory::TryCreate(_moduleFactory);
  if (FAILED(result)) return result;

  result = Timer::TryCreate(TIMER_SAMPLES, _timer);
  if (FAILED(result)) return result;

  _debugOverlay = MakeCom<Debug>(_timer);

  // map essential directories to the vfs
  // ensure the vfs automatically enumerates zip files
  LOG("Registering Zip file VFS handler...", MGDF_LOG_LOW);
  _vfs->RegisterArchiveHandler(vfs::zip::CreateZipArchiveHandlerImpl());

  // ensure the vfs enumerates any custom defined archive formats
  LOG("Registering custom archive VFS handlers...", MGDF_LOG_LOW);
  UINT64 length = 0;
  result = _moduleFactory->GetCustomArchiveHandlers(
      nullptr, &length, static_cast<IMGDFSimHost *>(this));
  if (FAILED(result)) {
    LOG("Failed to register custom archive VFS handlers", MGDF_LOG_ERROR);
    return result;
  }
  if (length > 0) {
    ComArray<IMGDFArchiveHandler> handlers(length);
    result = _moduleFactory->GetCustomArchiveHandlers(
        handlers.Data(), &length, static_cast<IMGDFSimHost *>(this));
    if (FAILED(result)) {
      LOG("Failed to register custom archive VFS handlers", MGDF_LOG_ERROR);
      return result;
    }

    for (const auto handler : handlers) {
      _vfs->RegisterArchiveHandler(handler);
    }
    LOG("Registered " << length << " custom archive VFS handlers",
        MGDF_LOG_LOW);
  } else {
    LOG("No custom archive VFS handlers to be registered", MGDF_LOG_LOW);
  }

  // enumerate the current games content directory
  LOG("Mounting content directory into VFS...", MGDF_LOG_LOW);
  if (!_vfs->Mount(Resources::Instance().ContentDir().c_str())) {
    LOG("Failed to mount content directory into VFS...", MGDF_LOG_ERROR);
    return E_FAIL;
  }

  auto working = Resources::Instance().WorkingDir();
  LOG("Mounting working directory \'" << Resources::ToString(working)
                                      << "\' into VFS",
      MGDF_LOG_LOW);
  _workingVfs = MakeCom<vfs::WriteableVirtualFileSystem>(working.c_str());

  // set the initial sound volumes
  if (_sound) {
    LOG("Setting initial volume...", MGDF_LOG_HIGH);
    std::string pref;
    ComObject<IMGDFGame> game = _game.As<IMGDFGame>();
    if (GetPreference(game, PreferenceConstants::SOUND_VOLUME, pref)) {
      _sound->SetSoundVolume(FromString<float>(pref));
    }

    if (GetPreference(game, PreferenceConstants::MUSIC_VOLUME, pref)) {
      _sound->SetStreamVolume(FromString<float>(pref));
    }
  }

  LOG("Initialised host components successfully", MGDF_LOG_LOW);
  return S_OK;
}

Host::~Host(void) {
  _ASSERTE(_references == 0UL);
  for (auto &it : _metrics) {
    it.second->Release();
  }
  LOG("Uninitialised host successfully", MGDF_LOG_LOW);
}

void Host::GetDebug(IMGDFDebug **debug) {
  _debugOverlay.AddRawRef<IMGDFDebug>(debug);
}

ComObject<Debug> Host::GetDebugImpl() { return _debugOverlay; }

ComObject<RenderSettingsManager> Host::GetRenderSettingsImpl() {
  return _renderSettings;
}

ComObject<input::IInputManagerComponent> Host::GetInputManagerImpl() {
  return _input;
}

UINT64 Host::GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                           UINT64 *featureLevelsSize) {
  return _moduleFactory->GetCompatibleFeatureLevels(levels, featureLevelsSize);
}

/**
create and initialize a new module
*/
void Host::STCreateModule() {
  if (!_module) {
    std::string error;
    if (_moduleFactory->GetLastError(error)) {
      FATALERROR(this, error);
    }

    // create the module
    if (FAILED(_moduleFactory->GetModule(_module))) {
      FATALERROR(this, "Unable to create module class");
    }

    // This is where additional QueryInterface checks should go if
    // IModule gets extended to determine support for new methods

    // init the module
    ClearWorkingDirectory();
    if (!_module->STNew(this)) {
      FATALERROR(this, "Error initialising module");
    }
  }
}

void Host::STUpdate(double simulationTime, HostStats &stats) {
  bool exp = true;
  if (_module && _shutdownQueued.compare_exchange_strong(exp, false)) {
    LOG("Calling module STShutDown...", MGDF_LOG_MEDIUM);
    _module->STShutDown(this);
  }

  const LARGE_INTEGER inputStart = _timer->GetCurrentTimeTicks();
  _input->ProcessSim();
  const LARGE_INTEGER inputEnd = _timer->GetCurrentTimeTicks();

  const LARGE_INTEGER audioStart = _timer->GetCurrentTimeTicks();
  if (_sound) _sound->Update();
  const LARGE_INTEGER audioEnd = _timer->GetCurrentTimeTicks();

  stats.AppendSimInputAndAudioTimes(
      _timer->ConvertDifferenceToSeconds(inputEnd, inputStart),
      _timer->ConvertDifferenceToSeconds(audioEnd, audioStart));

  if (_module) {
    LOG("Calling module STUpdate...", MGDF_LOG_HIGH);
    if (!_module->STUpdate(this, simulationTime)) {
      FATALERROR(this, "Error updating scene in module");
    }
  }

  std::lock_guard lock(_metricMutex);
  stats.UpdateMetrics(_metrics);
}

void Host::STDisposeModule() {
  LOG("Releasing module...", MGDF_LOG_MEDIUM);
  _module.Clear();
}

void Host::RTBeforeFirstDraw() {
  if (_module) {
    LOG("Calling module RTBeforeFirstDraw...", MGDF_LOG_MEDIUM);
    if (!_module->RTBeforeFirstDraw(this)) {
      FATALERROR(this, "Error in before first draw in module");
    }
  }
}

void Host::RTBeforeDeviceReset() {
  if (_module) {
    LOG("Calling module RTBeforeDeviceReset...", MGDF_LOG_MEDIUM);
    if (!_module->RTBeforeDeviceReset(this)) {
      FATALERROR(this, "Error in before device reset in module");
    }
  }
  _timer->BeforeDeviceReset();
  _d2dDevice.Clear();
  _d3dDevice.Clear();
}

void Host::QueueDeviceReset() {
  LOG("Module ready for Device Reset...", MGDF_LOG_MEDIUM);
  _deviceResetHandler();
}

void Host::RTDeviceReset() {
  if (_module) {
    LOG("Calling module RTDeviceReset...", MGDF_LOG_MEDIUM);
    if (!_module->RTDeviceReset(this)) {
      FATALERROR(this, "Error in device reset in module");
    }
  }
}

void Host::RTSetDevices(HWND window, const ComObject<ID3D11Device> &d3dDevice,
                        const ComObject<ID2D1Device> &d2dDevice,
                        const ComObject<IDXGIAdapter1> &adapter) {
  LOG("Initializing render settings and GPU timers...", MGDF_LOG_LOW);
  _renderSettings->InitFromDevice(window, d3dDevice, adapter);
  _timer->InitFromDevice(d3dDevice, GPU_TIMER_BUFFER);

  if (_renderSettings->GetAdaptorModeCount() == 0) {
    FATALERROR(this, "No compatible adaptor modes found");
  }

  if (!_d3dDevice) {
    LOG("Loading Render settings...", MGDF_LOG_LOW);
    auto game = _game.As<IMGDFGame>();
    _renderSettings->LoadPreferences(game);
  }

  _d2dDevice = d2dDevice;
  _d3dDevice = d3dDevice;
}

void Host::RTDraw(double alpha) {
  _timer->Begin();
  if (_module) {
    LOG("Calling module RTDraw...", MGDF_LOG_HIGH);
    if (!_module->RTDraw(this, alpha)) {
      FATALERROR(this, "Error drawing scene in module");
    }
  }
  _timer->End();
}

void Host::RTBeforeBackBufferChange() {
  _backBuffer.Clear();
  _depthStencilBuffer.Clear();
  if (_module) {
    LOG("Calling module RTBeforeBackBufferChange...", MGDF_LOG_MEDIUM);
    if (!_module->RTBeforeBackBufferChange(this)) {
      FATALERROR(this, "Error handling before back buffer change in module");
    }
  }
}

void Host::RTBackBufferChange(
    const ComObject<ID3D11Texture2D> &backBuffer,
    const ComObject<ID3D11Texture2D> &depthStencilBuffer) {
  _backBuffer = backBuffer;
  _depthStencilBuffer = depthStencilBuffer;
  if (_module) {
    LOG("Calling module RTBackBufferChange...", MGDF_LOG_MEDIUM);
    if (!_module->RTBackBufferChange(this)) {
      FATALERROR(this, "Error handling back buffer change in module");
    }
  }
}

void Host::GetBackBuffer(ID3D11Texture2D **backBuffer) {
  _backBuffer.AddRawRef(backBuffer);
}

void Host::GetDepthStencilBuffer(ID3D11Texture2D **stencilBuffer) {
  _depthStencilBuffer.AddRawRef(stencilBuffer);
}

void Host::GetBackBufferDescription(D3D11_TEXTURE2D_DESC *backBufferDesc,
                                    D3D11_TEXTURE2D_DESC *depthStencilDesc) {
  if (backBufferDesc) {
    _backBuffer->GetDesc(backBufferDesc);
  }
  if (depthStencilDesc) {
    _depthStencilBuffer->GetDesc(depthStencilDesc);
  }
}

void Host::GetD3DDevice(ID3D11Device **device) { _d3dDevice.AddRawRef(device); }

void Host::GetD2DDevice(ID2D1Device **device) { _d2dDevice.AddRawRef(device); }

void Host::GetRenderSettings(IMGDFRenderSettingsManager **settings) {
  _renderSettings.AddRawRef(settings);
}

BOOL Host::SetBackBufferRenderTarget(ID2D1DeviceContext *context) {
  if (!context) return false;

  LOG("Setting D2D device context render target to backbuffer...",
      MGDF_LOG_HIGH);
  D2D1_PIXEL_FORMAT pixelFormat;
  pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
  pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

  D2D1_BITMAP_PROPERTIES1 bitmapProperties;
  bitmapProperties.bitmapOptions =
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
  bitmapProperties.pixelFormat = pixelFormat;
  bitmapProperties.dpiX = 0;
  bitmapProperties.dpiX = 0;
  bitmapProperties.colorContext = nullptr;

  ComObject<IDXGISurface1> dxgiSurface;
  if (!FAILED(
          _backBuffer->QueryInterface<IDXGISurface1>(dxgiSurface.Assign()))) {
    ComObject<ID2D1Bitmap1> bitmap;
    if (!FAILED(context->CreateBitmapFromDxgiSurface(
            dxgiSurface, bitmapProperties, bitmap.Assign()))) {
      context->SetTarget(bitmap);
      return true;
    }
  }
  return false;
}

void Host::FatalError(const char *sender, const char *message) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (sender && message) {
    std::ostringstream ss;
    ss << "FATAL ERROR: " << message;
    Logger::Instance().Log(sender, ss.str().c_str(), MGDF_LOG_ERROR);
  }
  LOG("Notified of fatal error, telling module to panic", MGDF_LOG_ERROR);
  Logger::Instance().Flush();

  if (_module) {
    _module->Panic();
  }

  _fatalErrorHandler(
      sender ? sender : "",
      message ? message : "");  // signal any callbacks to the fatal error event

  TerminateProcess(GetCurrentProcess(), 1);
}

const MGDFVersion *Host::GetMGDFVersion() { return &_version; }

void Host::SetLoggingLevel(MGDFLogLevel level) {
  Logger::Instance().SetLoggingLevel(level);
}
MGDFLogLevel Host::GetLoggingLevel() {
  return Logger::Instance().GetLoggingLevel();
}
void Host::Log(const char *sender, const char *message, MGDFLogLevel level) {
  Logger::Instance().Log(sender, message, level);
}

void Host::GetTimer(IMGDFTimer **timer) { _timer.AddRawRef(timer); }

void Host::QueueShutDown() { _shutdownQueued.store(true); }

void Host::ShutDown() {
  _shutDownHandler();  // message the shutdown callback
}

void Host::GetSaves(IMGDFSaveManager **saves) {
  _saves.AddRawRef<IMGDFSaveManager>(saves);
}

void Host::GetGame(IMGDFGame **game) { _game.AddRawRef<IMGDFGame>(game); }

void Host::GetStatistics(IMGDFStatisticsManager **statistics) {
  _stats.AddRawRef<IMGDFStatisticsManager>(statistics);
}

void Host::GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) {
  _vfs.AddRawRef<IMGDFReadOnlyVirtualFileSystem>(vfs);
}

void Host::GetWorkingVFS(IMGDFWriteableVirtualFileSystem **vfs) {
  _workingVfs.AddRawRef<IMGDFWriteableVirtualFileSystem>(vfs);
}

void Host::GetInput(IMGDFInputManager **input) {
  _input.AddRawRef<IMGDFInputManager>(input);
}

void Host::GetSound(IMGDFSoundManager **sound) {
  _sound.AddRawRef<IMGDFSoundManager>(sound);
}

void Host::ClearWorkingDirectory() {
  LOG("Clearing working directory...", MGDF_LOG_HIGH);
  path workingDir(Resources::Instance().WorkingDir());
  if (exists(workingDir)) {
    remove_all(workingDir);
  } else {
    create_directories(workingDir);
  }
}

HRESULT Host::CreateCPUCounter(IMGDFMetric *metric,
                               IMGDFPerformanceCounter **counter) {
  return _timer->CreateCPUCounter(metric, counter);
}

HRESULT Host::CreateGPUCounter(IMGDFMetric *metric,
                               ID3D11DeviceContext *context,
                               IMGDFPerformanceCounter **counter) {
  return _timer->CreateGPUCounter(metric, context, counter);
}

HRESULT Host::CreateCounterMetric(const small *name, const small *description,
                                  IMGDFMetric **metric) {
  if (!name || !description) return E_FAIL;
  return CreateMetric<CounterMetric>(
      name, metric, [=]() { return new CounterMetric(name, description); });
}

HRESULT Host::CreateGaugeMetric(const small *name, const small *description,
                                IMGDFMetric **metric) {
  if (!name || !description) return E_FAIL;
  return CreateMetric<GaugeMetric>(
      name, metric, [=]() { return new GaugeMetric(name, description); });
}

HRESULT Host::CreateHistogramMetric(const small *name, const small *description,
                                    const double *buckets,
                                    const UINT64 bucketCount,
                                    IMGDFMetric **metric) {
  if (!name || !description || (!buckets && bucketCount)) return E_FAIL;
  return CreateMetric<HistogramMetric>(name, metric, [=]() {
    return new HistogramMetric(name, description, buckets, bucketCount);
  });
}

}  // namespace core
}  // namespace MGDF