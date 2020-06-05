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
      _stats(MakeCom<StatisticsManager>()),
      _renderSettings(MakeCom<RenderSettingsManager>()),
      _saves(MakeCom<SaveManager>(game, components.Storage)),
      _references(1UL),
      _d3dDevice(nullptr),
      _d3dContext(nullptr),
      _d2dDevice(nullptr),
      _depthStencilBuffer(nullptr),
      _backBuffer(nullptr) {
  _shutdownQueued.store(false);
  _ASSERTE(game);
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
  if (riid == IID_IUnknown || riid == __uuidof(ILogger) ||
      riid == __uuidof(ISimHost) || riid == __uuidof(IRenderHost) ||
      riid == __uuidof(ICommonHost)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
};

HRESULT Host::Init() {
  LOG("Creating Module factory...", LOG_LOW);
  HRESULT result = ModuleFactory::TryCreate(_moduleFactory);
  if (FAILED(result)) return result;

  result = Timer::TryCreate(TIMER_SAMPLES, _timer);
  if (FAILED(result)) return result;

  _debugOverlay = MakeCom<Debug>(_timer);

  // map essential directories to the vfs
  // ensure the vfs automatically enumerates zip files
  LOG("Registering Zip file VFS handler...", LOG_LOW);
  _vfs->RegisterArchiveHandler(vfs::zip::CreateZipArchiveHandlerImpl());

  // ensure the vfs enumerates any custom defined archive formats
  LOG("Registering custom archive VFS handlers...", LOG_LOW);
  UINT32 length = 0;
  result = _moduleFactory->GetCustomArchiveHandlers(
      nullptr, &length, reinterpret_cast<ILogger *>(this));
  if (FAILED(result)) {
    LOG("Failed to register custom archive VFS handlers", LOG_ERROR);
    return result;
  }
  if (length > 0) {
    ComArray<IArchiveHandler> handlers(length);
    result = _moduleFactory->GetCustomArchiveHandlers(
        handlers.Data(), &length, reinterpret_cast<ILogger *>(this));
    if (FAILED(result)) {
      LOG("Failed to register custom archive VFS handlers", LOG_ERROR);
      return result;
    }

    for (const auto handler : handlers) {
      _vfs->RegisterArchiveHandler(handler);
    }
    LOG("Registered " << length << " custom archive VFS handlers", LOG_LOW);
  } else {
    LOG("No custom archive VFS handlers to be registered", LOG_LOW);
  }

  // enumerate the current games content directory
  LOG("Mounting content directory into VFS...", LOG_LOW);
  if (!_vfs->Mount(Resources::Instance().ContentDir().c_str())) {
    LOG("Failed to mount content directory into VFS...", LOG_ERROR);
    return E_FAIL;
  }

  // set the initial sound volumes
  if (_sound) {
    LOG("Setting initial volume...", LOG_HIGH);
    std::string pref;
    ComObject<IGame> game = _game.As<IGame>();
    if (GetPreference(game, PreferenceConstants::SOUND_VOLUME, pref)) {
      _sound->SetSoundVolume(FromString<float>(pref));
    }

    if (GetPreference(game, PreferenceConstants::MUSIC_VOLUME, pref)) {
      _sound->SetStreamVolume(FromString<float>(pref));
    }
  }

  LOG("Initialised host components successfully", LOG_LOW);
  return S_OK;
}

Host::~Host(void) {
  _ASSERTE(_references == 0UL);
  LOG("Uninitialised host successfully", LOG_LOW);
}

void Host::GetDebug(IDebug **debug) {
  _debugOverlay.AddRawRef<MGDF::IDebug>(debug);
}

ComObject<Debug> Host::GetDebugImpl() { return _debugOverlay; }

ComObject<RenderSettingsManager> Host::GetRenderSettingsImpl() {
  return _renderSettings;
}

ComObject<input::IInputManagerComponent> Host::GetInputManagerImpl() {
  return _input;
}

UINT32 Host::GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                           UINT32 *featureLevelsSize) {
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
    if (!_module->STNew(this, Resources::Instance().WorkingDir().c_str())) {
      FATALERROR(this, "Error initialising module");
    }
  }
}

void Host::STUpdate(double simulationTime, HostStats &stats) {
  bool exp = true;
  if (_module && _shutdownQueued.compare_exchange_strong(exp, false)) {
    LOG("Calling module STShutDown...", LOG_MEDIUM);
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
    LOG("Calling module STUpdate...", LOG_HIGH);
    if (!_module->STUpdate(this, simulationTime)) {
      FATALERROR(this, "Error updating scene in module");
    }
  }
}

void Host::STDisposeModule() {
  LOG("Releasing module...", LOG_MEDIUM);
  _module.Clear();
}

void Host::RTBeforeFirstDraw() {
  if (_module) {
    LOG("Calling module RTBeforeFirstDraw...", LOG_MEDIUM);
    if (!_module->RTBeforeFirstDraw(this)) {
      FATALERROR(this, "Error in before first draw in module");
    }
  }
}

void Host::RTBeforeDeviceReset() {
  _d3dContext.Clear();
  _d2dDevice.Clear();
  _d3dDevice.Clear();
  _timer->BeforeDeviceReset();
  if (_module) {
    LOG("Calling module RTBeforeDeviceReset...", LOG_MEDIUM);
    if (!_module->RTBeforeDeviceReset(this)) {
      FATALERROR(this, "Error in before device reset in module");
    }
  }
}

void Host::RTSetDevices(HWND window, const ComObject<ID3D11Device> &d3dDevice,
                        const ComObject<ID2D1Device> &d2dDevice,
                        const ComObject<IDXGIAdapter1> &adapter) {
  LOG("Initializing render settings and GPU timers...", LOG_LOW);
  _renderSettings->InitFromDevice(window, d3dDevice, adapter);
  _timer->InitFromDevice(d3dDevice, GPU_TIMER_BUFFER);

  if (_renderSettings->GetAdaptorModeCount() == 0) {
    FATALERROR(this, "No compatible adaptor modes found");
  }

  if (!_d3dDevice) {
    LOG("Loading Render settings...", LOG_LOW);
    auto game = _game.As<IGame>();
    _renderSettings->LoadPreferences(game);
  }

  _d2dDevice = d2dDevice;
  _d3dDevice = d3dDevice;
  _d3dDevice->GetImmediateContext(_d3dContext.Assign());
  _ASSERTE(_d3dContext);
}

void Host::RTDraw(double alpha) {
  _timer->Begin();
  if (_module) {
    LOG("Calling module RTDraw...", LOG_HIGH);
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
    LOG("Calling module RTBeforeBackBufferChange...", LOG_MEDIUM);
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
    LOG("Calling module RTBackBufferChange...", LOG_MEDIUM);
    if (!_module->RTBackBufferChange(this)) {
      FATALERROR(this, "Error handling back buffer change in module");
    }
  }
}

ID3D11Texture2D *Host::GetBackBuffer() const { return _backBuffer; }

ID3D11Texture2D *Host::GetDepthStencilBuffer() const { return _backBuffer; }

void Host::GetBackBufferDescription(
    D3D11_TEXTURE2D_DESC *backBufferDesc,
    D3D11_TEXTURE2D_DESC *depthStencilDesc) const {
  if (backBufferDesc) {
    _backBuffer->GetDesc(backBufferDesc);
  }
  if (depthStencilDesc) {
    _depthStencilBuffer->GetDesc(depthStencilDesc);
  }
}

ID3D11Device *Host::GetD3DDevice() const { return _d3dDevice; }

ID3D11DeviceContext *Host::GetD3DImmediateContext() const {
  return _d3dContext;
}

ID2D1Device *Host::GetD2DDevice() const { return _d2dDevice; }

void Host::GetRenderSettings(IRenderSettingsManager **settings) {
  _renderSettings.AddRawRef(settings);
}

bool Host::SetBackBufferRenderTarget(ID2D1DeviceContext *context) {
  if (!context) return false;

  LOG("Setting D2D device context render target to backbuffer...", LOG_HIGH);
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
    Logger::Instance().Log(sender, ss.str().c_str(), LOG_ERROR);
  }
  LOG("Notified of fatal error, telling module to panic", LOG_ERROR);
  Logger::Instance().Flush();

  if (_module) {
    _module->Panic();
  }

  _fatalErrorHandler(
      sender ? sender : "",
      message ? message : "");  // signal any callbacks to the fatal error event

  TerminateProcess(GetCurrentProcess(), 1);
}

const Version *Host::GetMGDFVersion() const { return &_version; }

void Host::SetLoggingLevel(LogLevel level) {
  Logger::Instance().SetLoggingLevel(level);
}
LogLevel Host::GetLoggingLevel() const {
  return Logger::Instance().GetLoggingLevel();
}
void Host::Log(const char *sender, const char *message, LogLevel level) {
  Logger::Instance().Log(sender, message, level);
}

void Host::GetTimer(ITimer **timer) { _timer.AddRawRef(timer); }

void Host::QueueShutDown() { _shutdownQueued.store(true); }

void Host::ShutDown() {
  _shutDownHandler();  // message the shutdown callback
}

void Host::GetSaves(ISaveManager **saves) {
  _saves.AddRawRef<ISaveManager>(saves);
}

void Host::GetGame(IGame **game) { _game.AddRawRef<IGame>(game); }

void Host::GetStatistics(IStatisticsManager **statistics) {
  _stats.AddRawRef(statistics);
}

void Host::GetVFS(IVirtualFileSystem **vfs) {
  _vfs.AddRawRef<IVirtualFileSystem>(vfs);
}

void Host::GetInput(IInputManager **input) {
  _input.AddRawRef<IInputManager>(input);
}

void Host::GetSound(ISoundManager **sound) {
  _sound.AddRawRef<ISoundManager>(sound);
}

void Host::ClearWorkingDirectory() {
  LOG("Clearing working directory...", LOG_HIGH);
  path workingDir(Resources::Instance().WorkingDir());
  if (exists(workingDir)) {
    remove_all(workingDir);
  } else {
    create_directories(workingDir);
  }
}

HRESULT Host::CreateCPUCounter(const char *name,
                               IPerformanceCounter **counter) {
  return _timer->CreateCPUCounter(name, counter);
}

HRESULT Host::CreateGPUCounter(const char *name,
                               IPerformanceCounter **counter) {
  return _timer->CreateGPUCounter(name, counter);
}

const char *Host::GetErrorDescription(MGDFError err) const {
  static std::string ok("No error");
  static std::string allocatingBuffer("Error allocating audio buffer");
  static std::string noFreeSources("No free audio sources");
  static std::string vorbisFailed("Failed to load vorbis audio library");
  static std::string invalidFormat("Invalid audio file format");
  static std::string invalidArchive("Invalid archive file");
  static std::string invalidFile("Not a valid VFS file");
  static std::string noPending(
      "This save name does not match any pending save created by "
      "BeginSave");
  static std::string invalidSave(
      "Invalid save name - only alphanumeric characters and the space "
      "character are permitted");
  static std::string archiveToLarge(
      "Archive file is too large - archive files cannot be over 4GB in "
      "size");
  static std::string fileInUse("File is already open for reading elsewhere");
  static std::string bufferTooSmall(
      "Target buffer is too small to hold all required data");
  static std::string fatal("Fatal error - shutting down");
  static std::string folder("Cannot open a folder for reading");
  static std::string gpuTimer(
      "GPU Timers are not supported at this DirectX feature level, only "
      "10 and "
      "up is supported");
  static std::string invalidTimerName("Timer names cannot be null");
  static std::string invalidJson("Invalid JSON data");
  static std::string invalidParam("Invalid parameter format");
  static std::string audioInitFailed("Failed to initialize audio system");
  static std::string cpuTimer(
      "High resolution timers are unsupported on this system");
  static std::string unknown("Unknown error");

  switch (err) {
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

const char *Host::GetErrorString(MGDFError err) const {
  static std::string ok("MGDF_OK");
  static std::string allocatingBuffer("MGDF_ERR_ERROR_ALLOCATING_BUFFER");
  static std::string noFreeSources("MGDF_ERR_NO_FREE_SOURCES");
  static std::string vorbisFailed("MGDF_ERR_VORBIS_LIB_LOAD_FAILED");
  static std::string invalidFormat("MGDF_ERR_INVALID_FORMAT");
  static std::string invalidArchive("MGDF_ERR_INVALID_ARCHIVE_FILE");
  static std::string invalidFile("MGDF_ERR_INVALID_FILE");
  static std::string noPending("MGDF_ERR_NO_PENDING_SAVE");
  static std::string invalidSave("MGDF_ERR_INVALID_SAVE_NAME");
  static std::string archiveToLarge("MGDF_ERR_ARCHIVE_FILE_TOO_LARGE");
  static std::string fileInUse("MGDF_ERR_FILE_IN_USE");
  static std::string bufferTooSmall("MGDF_ERR_BUFFER_TOO_SMALL");
  static std::string fatal("MGDF_ERR_FATAL");
  static std::string folder("MGDF_ERR_IS_FOLDER");
  static std::string gpuTimer("MGDF_ERR_GPU_TIMER_UNSUPPORTED");
  static std::string invalidTimerName("MGDF_ERR_INVALID_TIMER_NAME");
  static std::string invalidJson("MGDF_ERR_INVALID_JSON");
  static std::string invalidParam("MGDF_ERR_INVALID_PARAMETER");
  static std::string audioInitFailed("MGDF_ERR_AUDIO_INIT_FAILED");
  static std::string cpuTimer("MGDF_ERR_CPU_TIMER_UNSUPPORTED");
  static std::string unknown("MGDF_ERR_UNKNOWN");

  switch (err) {
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

}  // namespace core
}  // namespace MGDF