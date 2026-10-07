#include "StdAfx.h"

#include "MGDFHostImpl.hpp"

#include <filesystem>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFPreferenceConstants.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../vfs/archive/zip/ZipArchiveHandlerImpl.hpp"
#include "MGDFGraphicsRequirements.hpp"
#include "MGDFMetrics.hpp"
#include "MGDFNetworkImpl.hpp"
#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

static const std::string S_EMPTY("");

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
      _network(components.Network),
      _stats(MakeCom<StatisticsManager>(components.Network, game->GetUid())),
      _metricsServer(components.Network),
      _renderSettings(MakeCom<RenderSettingsManager>()),
      _saves(MakeCom<SaveManager>(game, components.VFS, components.Storage)),
      _references(1UL),
      _d3dDevice(nullptr),
      _depthStencilBuffer(nullptr),
      _backBuffer(nullptr) {
  _shutdownQueued.store(false);
  _ASSERTE(game);

  auto statsEndpointOverride = ParameterManager::Instance().GetParameter(
      ParameterConstants::STATISTICS_ENDPOINT_OVERRIDE);
  if (statsEndpointOverride) {
    _stats->SetRemoteEndpoint(statsEndpointOverride);
  } else if (ParameterManager::Instance().HasParameter(
                 ParameterConstants::STATISTICS_ENABLED)) {
    _stats->SetRemoteEndpoint(_game->GetStatististicsService());
  }

  auto metricsPort = ParameterManager::Instance().GetParameter(
      ParameterConstants::METRICS_PORT);
  if (metricsPort && atoi(metricsPort)) {
    const auto parsedPort = atoi(metricsPort);
    if (parsedPort) {
      _metricsServer.Listen(parsedPort);
    }
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
  *ppvObject = nullptr;
  if (riid == IID_IUnknown || riid == __uuidof(IMGDFLogger) ||
      riid == __uuidof(IMGDFCommonHost) || riid == __uuidof(IMGDFRenderHost)) {
    *ppvObject = static_cast<IMGDFRenderHost *>(this);
  } else if (riid == __uuidof(IMGDFSimHost)) {
    *ppvObject = static_cast<IMGDFSimHost *>(this);
  } else if (_graphicsAPI == MGDF_GRAPHICS_API_D3D11 &&
             riid == __uuidof(IMGDFD3D11Host)) {
    *ppvObject = static_cast<IMGDFD3D11Host *>(this);
  } else if (_graphicsAPI == MGDF_GRAPHICS_API_D3D11 &&
             riid == __uuidof(IMGDFD3D11RenderHost)) {
    *ppvObject = static_cast<IMGDFD3D11RenderHost *>(this);
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
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
#pragma warning(push)
#pragma warning(disable : 26474)
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
#pragma warning(pop)

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
  if (!vfs::CreateWriteableVirtualFileSystemComponent(working, _workingVfs)) {
    LOG("Failed to mount working directory into VFS...", MGDF_LOG_ERROR);
    return E_FAIL;
  }

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
  STDisposeModule();
  for (auto &it : _metrics) {
    it.second->Release();
  }
  _sound->Stop();
  _network->Stop();
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

void Host::InitGraphics() {
  if (FAILED(_moduleFactory->GetGraphicsRequirements(_graphicsRequirements))) {
    FATALERROR(this, "Module GetGraphicsRequirements failed");
    return;
  }
  std::string preference;
  GetPreference(_game.As<IMGDFGame>(), PreferenceConstants::GRAPHICS_API,
                preference);
  std::string error;
  const HRESULT result =
      SelectGraphicsAPI(_graphicsRequirements, preference, _graphicsAPI, error);
  if (SUCCEEDED(result) || result == E_NOTIMPL) {
    LOG("Selected graphics API: "
            << (_graphicsAPI == MGDF_GRAPHICS_API_D3D11 ? "d3d11" : "d3d12"),
        MGDF_LOG_LOW);
  }
  if (FAILED(result)) {
    FATALERROR(this, error);
    return;
  }
  if (GetD3D11FeatureLevels(_graphicsRequirements.MinFeatureLevel).empty()) {
    FATALERROR(this,
               "The requested minimum feature level is not supported by D3D11");
  }
}

const MGDFGraphicsRequirements &Host::GetGraphicsRequirements() const {
  return _graphicsRequirements;
}

MGDFGraphicsAPI Host::GetGraphicsAPI() { return _graphicsAPI; }

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
    const HRESULT result = _moduleFactory->GetModule(_module);
    if (result == E_NOINTERFACE) {
      FATALERROR(this, "The module was built for another interface version");
      return;
    } else if (FAILED(result)) {
      FATALERROR(this, "Unable to create module class");
      return;
    }

    // init the module
    ClearWorkingDirectory();
    if (!_module->STNew(this)) {
      FATALERROR(this, "Error initialising module");
    }
  }
}

void Host::STUpdate(double simulationTime, HostMetrics &stats) {
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
  _metricsServer.UpdateResponse(_metrics);
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
  std::lock_guard lock(_deviceMutex);
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

void Host::RTShutDown() {
  if (_module) {
    LOG("Calling module RTShutdown...", MGDF_LOG_MEDIUM);
    _module->RTShutDown(this);
  }
  // release all device dependent resources now as the app framework will
  // uninitialize D3D (and report any remaining live objects in debug builds)
  // before this host is destroyed
  _backBuffer.Clear();
  _depthStencilBuffer.Clear();
  _timer->BeforeDeviceReset();
  std::lock_guard lock(_deviceMutex);
  _d3dDevice.Clear();
}

void Host::RTSetDevices(IRenderBackend &backend) {
  auto d3dDevice = backend.RTGetDevice().As<ID3D11Device>();
  _ASSERTE(d3dDevice);
  LOG("Initializing render settings and GPU timers...", MGDF_LOG_LOW);
  _renderSettings->InitFromDevice(d3dDevice);
  _timer->InitFromDevice(d3dDevice, GPU_TIMER_BUFFER);

  if (!_d3dDevice) {
    LOG("Loading Render settings...", MGDF_LOG_LOW);
    auto game = _game.As<IMGDFGame>();
    _renderSettings->LoadPreferences(game);
  }

  std::lock_guard lock(_deviceMutex);
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

void Host::RTAfterPresent() {
  if (_module) _module->RTAfterPresent(this);
}

void Host::RTBeforeBackBufferChange() {
  _backBufferInfo = {};
  _backBuffer.Clear();
  _depthStencilBuffer.Clear();
  if (_module) {
    LOG("Calling module RTBeforeBackBufferChange...", MGDF_LOG_MEDIUM);
    if (!_module->RTBeforeBackBufferChange(this)) {
      FATALERROR(this, "Error handling before back buffer change in module");
    }
  }
}

void Host::RTBackBufferChange(IRenderBackend &backend) {
  _backBufferInfo = backend.RTGetBackBufferInfo();
  _backBuffer = backend.RTGetBackBuffer().As<ID3D11Texture2D>();
  _depthStencilBuffer = backend.RTGetDepthStencilBuffer().As<ID3D11Texture2D>();
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

void Host::GetBackBufferInfo(MGDFBackBufferInfo *info) {
  *info = _backBufferInfo;
}

void Host::GetD3D11Device(ID3D11Device **device) {
  std::lock_guard lock(_deviceMutex);
  _d3dDevice.AddRawRef(device);
}

void Host::GetRenderSettings(IMGDFRenderSettingsManager **settings) {
  _renderSettings.AddRawRef(settings);
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

HRESULT Host::CreateHttpRequest(const small *url,
                                IMGDFHttpClientRequest **request) {
  if (!url) return E_FAIL;
  auto r = _network->CreateHttpRequest(url);
  auto com = MakeCom<HttpClientRequestImpl>(r);
  com.AddRawRef(request);
  return S_OK;
}

HRESULT __stdcall Host::CreateHttpRequestGroup(
    IMGDFHttpClientRequestGroup **group) {
  auto com = MakeCom<HttpClientRequestGroupImpl>();
  com.AddRawRef(group);
  return S_OK;
}

HRESULT __stdcall Host::CreateWebSocket(const small *url,
                                        IMGDFWebSocket **socket) {
  auto s = _network->CreateWebSocket(url);
  auto com = MakeCom<WebSocketImpl>(s);
  com.AddRawRef(socket);
  return S_OK;
}

HRESULT __stdcall Host::CreateWebServer(unsigned int port,
                                        const small *socketPath,
                                        IMGDFWebServer **server) {
  auto s = _network->CreateHttpServer(port, socketPath ? socketPath : S_EMPTY);
  auto com = MakeCom<WebServerImpl>(s);
  com.AddRawRef(server);
  return S_OK;
}

}  // namespace core
}  // namespace MGDF