#include "Stdafx.h"
#undef min
#undef max
#include <AL/alext.h>
#include <audioclient.h>
#include <avrt.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <future>
#include <thread>

#include "AudioOutput.hpp"

#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace MGDF::core::audio::openal_audio {
namespace {
int64_t QpcTime() {
  LARGE_INTEGER now, frequency;
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&frequency);
  return static_cast<int64_t>(static_cast<double>(now.QuadPart) *
                              AudioTicksPerSecond / frequency.QuadPart);
}
class WasapiEndpoint final : public OutputEndpoint {
 public:
  ~WasapiEndpoint() {
    if (_client) _client->Stop();
    if (Event) CloseHandle(Event);
  }
  HRESULT Open(IMMDevice *device) {
    LPWSTR id = nullptr;
    auto result = device->GetId(&id);
    if (FAILED(result)) return result;
    Id = id;
    CoTaskMemFree(id);
    result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              std::bit_cast<void **>(_client.Assign()));
    WAVEFORMATEX *mix = nullptr;
    if (SUCCEEDED(result)) result = _client->GetMixFormat(&mix);
    if (FAILED(result)) return result;
    if (!mix) return E_UNEXPECTED;
    const auto count = mix->nChannels;
    Channels = count >= 8 ? 8 : count >= 6 ? 6 : count == 4 ? 4 : 2;
    DWORD mask = Channels == 8   ? KSAUDIO_SPEAKER_7POINT1_SURROUND
                 : Channels == 6 ? KSAUDIO_SPEAKER_5POINT1_SURROUND
                 : Channels == 4 ? KSAUDIO_SPEAKER_QUAD
                                 : KSAUDIO_SPEAKER_STEREO;
    if (Channels == 6 && mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        mix->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
      const auto endpointMask =
          std::bit_cast<WAVEFORMATEXTENSIBLE *>(mix)->dwChannelMask;
      if (endpointMask == KSAUDIO_SPEAKER_5POINT1) mask = endpointMask;
    }
    CoTaskMemFree(mix);
    WAVEFORMATEXTENSIBLE format{};
    format.Format = {WAVE_FORMAT_EXTENSIBLE,
                     static_cast<WORD>(Channels),
                     MixRate,
                     MixRate * Channels * 4,
                     static_cast<WORD>(Channels * 4),
                     32,
                     22};
    format.Samples.wValidBitsPerSample = 32;
    format.dwChannelMask = mask;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    // OpenAL Soft defaults to three 960-frame periods at 48 kHz.
    result = _client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                                     AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                     AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                 600000, 0, &format.Format, nullptr);
    if (SUCCEEDED(result)) result = _client->GetBufferSize(&BufferFrames);
    Event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!Event) return HRESULT_FROM_WIN32(GetLastError());
    if (SUCCEEDED(result)) result = _client->SetEventHandle(Event);
    if (SUCCEEDED(result))
      result = _client->GetService(IID_PPV_ARGS(_render.Assign()));
    if (SUCCEEDED(result))
      result = _client->GetService(IID_PPV_ARGS(_clock.Assign()));
    return result;
  }
  HRESULT Start() final { return _client->Start(); }
  HRESULT Padding(UINT32 &frames) final {
    return _client->GetCurrentPadding(&frames);
  }
  HRESULT Buffer(UINT32 frames, float **data) final {
    return _render->GetBuffer(frames, std::bit_cast<BYTE **>(data));
  }
  HRESULT Submit(UINT32 frames) final {
    return _render->ReleaseBuffer(frames, 0);
  }
  HRESULT Position(UINT64 &position, UINT64 &qpc, UINT64 &frequency) final {
    const auto result = _clock->GetFrequency(&frequency);
    return FAILED(result) ? result : _clock->GetPosition(&position, &qpc);
  }

 private:
  ComObject<IAudioClient> _client;
  ComObject<IAudioRenderClient> _render;
  ComObject<IAudioClock> _clock;
};
class EndpointNotifications final : public ComBase<IMMNotificationClient> {
 public:
  explicit EndpointNotifications(HANDLE changed) : _changed(changed) {}
  HRESULT __stdcall OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                           LPCWSTR) final {
    if (flow == eRender && role == eMultimedia) SetEvent(_changed);
    return S_OK;
  }
  HRESULT __stdcall OnDeviceStateChanged(LPCWSTR, DWORD) final {
    SetEvent(_changed);
    return S_OK;
  }
  HRESULT __stdcall OnDeviceAdded(LPCWSTR) final { return S_OK; }
  HRESULT __stdcall OnDeviceRemoved(LPCWSTR) final {
    SetEvent(_changed);
    return S_OK;
  }
  HRESULT __stdcall OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) final {
    SetEvent(_changed);
    return S_OK;
  }

 private:
  HANDLE _changed;
};
class WasapiSource final : public OutputEndpointSource {
 public:
  WasapiSource() {
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL,
                                   IID_PPV_ARGS(_devices.Assign())))) {
      _notifications = MakeCom<EndpointNotifications>(Changed);
      _devices->RegisterEndpointNotificationCallback(_notifications);
    }
  }
  ~WasapiSource() {
    if (_devices && _notifications)
      _devices->UnregisterEndpointNotificationCallback(_notifications);
  }
  ComObject<OutputEndpoint> Open() final {
    if (!_devices) return {};
    ComObject<IMMDevice> device;
    if (FAILED(_devices->GetDefaultAudioEndpoint(eRender, eMultimedia,
                                                 device.Assign())))
      return {};
    auto endpoint = MakeCom<WasapiEndpoint>();
    if (FAILED(endpoint->Open(device))) return {};
    return MakeComFromPtr<OutputEndpoint>(endpoint.Get());
  }

 private:
  ComObject<IMMDeviceEnumerator> _devices;
  ComObject<EndpointNotifications> _notifications;
};
class WavOutput {
 public:
  HRESULT Open(const std::filesystem::path &path) {
    std::error_code error;
    if (path.has_parent_path())
      std::filesystem::create_directories(path.parent_path(), error);
    if (error) return HRESULT_FROM_WIN32(error.value());
    _file.open(path, std::ios::binary | std::ios::trunc);
    if (!_file) return HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
    Header();
    return S_OK;
  }
  void Write(const float *samples, UINT32 frames) {
    _file.write(std::bit_cast<const char *>(samples),
                static_cast<std::streamsize>(frames) * 8);
    _bytes += frames * 8;
  }
  void Close() {
    if (!_file.is_open()) return;
    _file.seekp(0);
    Header();
    _file.close();
  }
  ~WavOutput() { Close(); }

 private:
  void Header() {
    const std::array<uint32_t, 11> header{
        0x46464952, _bytes + 36, 0x45564157,
        0x20746d66, 16,          (2u << 16) | WAVE_FORMAT_IEEE_FLOAT,
        MixRate,    MixRate * 8, (32u << 16) | 8,
        0x61746164, _bytes};
    _file.write(std::bit_cast<const char *>(header.data()), sizeof(header));
  }
  std::ofstream _file;
  uint32_t _bytes = 0;
};
}  // namespace
OutputEndpointSource::OutputEndpointSource()
    : Changed(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {}
OutputEndpointSource::~OutputEndpointSource() {
  if (Changed) CloseHandle(Changed);
}

struct AudioOutput::Impl {
  AudioOutput &Owner;
  ALCdevice *Device;
  bool Hrtf;
  ComObject<AudioCaptureSource> Capture;
  std::filesystem::path File;
  ComObject<OutputEndpointSource> Source;
  ComObject<OutputEndpoint> Endpoint;
  LPALCRENDERSAMPLESSOFT Render = nullptr;
  LPALCRESETDEVICESOFT Reset = nullptr;
  HANDLE StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  HANDLE Timer = CreateWaitableTimerExW(nullptr, nullptr,
                                        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                        TIMER_ALL_ACCESS);
  std::thread Thread;
  std::future<ComObject<OutputEndpoint>> Opening;
  bool OpenAgain = false;
  std::wstring EndpointId;
  UINT32 Channels = 2;
  UINT64 Rendered = 0, Submitted = 0;
  int64_t Pace = 0, Retry = 0, PublishedThrough = 0;
  WavOutput Wav;
  std::array<float, MixBlockFrames * 8> Mix{};

  Impl(AudioOutput &owner, ALCdevice *device, bool hrtf,
       const ComObject<AudioCaptureSource> &capture, std::filesystem::path file,
       const ComObject<OutputEndpointSource> &source)
      : Owner(owner),
        Device(device),
        Hrtf(hrtf),
        Capture(capture),
        File(std::move(file)),
        Source(source) {}
  ~Impl() {
    if (Timer) CloseHandle(Timer);
    if (StopEvent) CloseHandle(StopEvent);
  }
  bool Layout(UINT32 channels) {
    if (channels == Channels) return true;
    const ALCint layout = channels == 8   ? ALC_7POINT1_SOFT
                          : channels == 6 ? ALC_5POINT1_SOFT
                          : channels == 4 ? ALC_QUAD_SOFT
                                          : ALC_STEREO_SOFT;
    const ALCint attributes[]{
        ALC_FREQUENCY,
        MixRate,
        ALC_FORMAT_CHANNELS_SOFT,
        layout,
        ALC_FORMAT_TYPE_SOFT,
        ALC_FLOAT_SOFT,
        ALC_HRTF_SOFT,
        Hrtf ? ALC_TRUE : ALC_FALSE,
        ALC_OUTPUT_MODE_SOFT,
        Hrtf ? ALC_STEREO_HRTF_SOFT : ALC_STEREO_BASIC_SOFT,
        0};
    if (!Reset(Device, attributes)) return false;
    Channels = channels;
    return true;
  }
  void UseEndpoint(ComObject<OutputEndpoint> endpoint) {
    Endpoint = std::move(endpoint);
    const auto channels = Endpoint ? Endpoint->Channels : 2;
    if (!Layout(channels)) Endpoint.Clear();
    Submitted = 0;
    if (Endpoint) {
      float *silence = nullptr;
      const auto frames = Endpoint->BufferFrames;
      if (SUCCEEDED(Endpoint->Buffer(frames, &silence))) {
        std::fill_n(silence, size_t(frames) * Channels, 0.0f);
        if (SUCCEEDED(Endpoint->Submit(frames)) &&
            SUCCEEDED(Endpoint->Start())) {
          Submitted = frames;
          return;
        }
      }
      Endpoint.Clear();
      Layout(2);
    }
  }
  void Reopen() {
    if (Endpoint) {
      Endpoint.Clear();
      Layout(2);
      StartPaced();
    }
    if (!Source) return;
    if (Opening.valid()) {
      OpenAgain = true;
      return;
    }
    Retry = QpcTime() + AudioTicksPerSecond;
    // Endpoint activation can wait for a driver. Keep mixing while it opens.
    Opening = std::async(std::launch::async, [source = Source] {
      const auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      ComObject<OutputEndpoint> endpoint;
      try {
        endpoint = source->Open();
      } catch (...) {
      }
      if (SUCCEEDED(com)) CoUninitialize();
      return endpoint;
    });
  }
  void StartPaced() {
    const auto now = QpcTime();
    Pace = std::max(now, PublishedThrough);
    Retry = now + AudioTicksPerSecond;
  }
  void RenderBlock(float *output, UINT32 frames, int64_t playTime) {
    const auto begin = QpcTime();
    Render(Device, output, static_cast<ALCsizei>(frames));
    Capture->Publish(output, Channels, frames, Rendered, playTime);
    PublishedThrough = playTime + frames * AudioTicksPerSecond / MixRate;
    Rendered += frames;
    Owner.RenderMs.store(static_cast<float>(QpcTime() - begin) / 10000.0f,
                         std::memory_order_relaxed);
  }
  HRESULT FillEndpoint() {
    UINT32 padding = 0;
    auto result = Endpoint->Padding(padding);
    if (FAILED(result)) return result;
    UINT64 position = 0, qpc = 0, frequency = 0;
    result = Endpoint->Position(position, qpc, frequency);
    if (FAILED(result) || !frequency) return E_FAIL;
    const int64_t first =
        static_cast<int64_t>(qpc) +
        static_cast<int64_t>((static_cast<double>(Submitted) / MixRate -
                              static_cast<double>(position) / frequency) *
                             AudioTicksPerSecond);
    UINT32 remaining =
        Endpoint->BufferFrames - std::min(padding, Endpoint->BufferFrames);
    while (remaining) {
      const auto count = std::min(remaining, MixBlockFrames);
      float *data = nullptr;
      result = Endpoint->Buffer(count, &data);
      if (FAILED(result)) return result;
      const auto time =
          first + static_cast<int64_t>(
                      Endpoint->BufferFrames -
                      std::min(padding, Endpoint->BufferFrames) - remaining) *
                      AudioTicksPerSecond / MixRate;
      // A new endpoint can play before the last block from paced output ends.
      // Fill that interval with silence without changing either clock.
      const auto overlap = std::max<int64_t>(0, PublishedThrough - time);
      const auto silence = static_cast<UINT32>(std::min<int64_t>(
          count,
          (overlap * MixRate + AudioTicksPerSecond - 1) / AudioTicksPerSecond));
      std::fill_n(data, size_t(silence) * Channels, 0.0f);
      if (silence < count)
        RenderBlock(data + size_t(silence) * Channels, count - silence,
                    time + silence * AudioTicksPerSecond / MixRate);
      result = Endpoint->Submit(count);
      if (FAILED(result)) return result;
      Submitted += count;
      remaining -= count;
    }
    return S_OK;
  }
  HRESULT Open() {
    if (!Timer || !StopEvent) return HRESULT_FROM_WIN32(GetLastError());
    Render = std::bit_cast<LPALCRENDERSAMPLESSOFT>(
        alcGetProcAddress(Device, "alcRenderSamplesSOFT"));
    Reset = std::bit_cast<LPALCRESETDEVICESOFT>(
        alcGetProcAddress(Device, "alcResetDeviceSOFT"));
    if (!Render || !Reset) return E_NOTIMPL;
    if (!File.empty()) {
      const auto result = Wav.Open(File);
      if (FAILED(result)) return result;
    } else if (!Source)
      Source = MakeCom<WasapiSource>();
    if (Source) UseEndpoint(Source->Open());
    EndpointId = Endpoint ? Endpoint->Id : L"";
    StartPaced();
    return S_OK;
  }
  void Run() {
    DWORD task = 0;
    const auto mmcss = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task);
    while (WaitForSingleObject(StopEvent, 0) != WAIT_OBJECT_0) {
      if (Opening.valid() && Opening.wait_for(std::chrono::seconds(0)) ==
                                 std::future_status::ready) {
        auto endpoint = Opening.get();
        if (OpenAgain) {
          OpenAgain = false;
          Reopen();
        } else {
          UseEndpoint(std::move(endpoint));
          const auto id = Endpoint ? Endpoint->Id : L"";
          if (id != EndpointId) {
            EndpointId = id;
            Owner.DeviceChanges.fetch_add(1, std::memory_order_relaxed);
          }
          Retry = QpcTime() + AudioTicksPerSecond;
        }
      }
      if (Source && WaitForSingleObject(Source->Changed, 0) == WAIT_OBJECT_0)
        Reopen();
      if (Endpoint) {
        if (FAILED(FillEndpoint())) {
          Reopen();
          continue;
        }
        HANDLE events[]{StopEvent, Source->Changed, Endpoint->Event};
        const auto wait = WaitForMultipleObjects(3, events, FALSE, 100);
        if (wait == WAIT_OBJECT_0) break;
        if (wait == WAIT_OBJECT_0 + 1 || wait == WAIT_TIMEOUT ||
            wait == WAIT_FAILED)
          Reopen();
      } else {
        const auto now = QpcTime();
        if (now < Pace) {
          LARGE_INTEGER due;
          due.QuadPart = -(Pace - now);
          SetWaitableTimer(Timer, &due, 0, nullptr, nullptr, FALSE);
          HANDLE events[]{StopEvent, Timer};
          if (WaitForMultipleObjects(2, events, FALSE, INFINITE) ==
              WAIT_OBJECT_0)
            break;
        }
        RenderBlock(Mix.data(), MixBlockFrames, Pace);
        if (!File.empty()) Wav.Write(Mix.data(), MixBlockFrames);
        Pace += MixBlockFrames * AudioTicksPerSecond / MixRate;
        if (Source && !Opening.valid() && now >= Retry) {
          Reopen();
        }
      }
    }
    Endpoint.Clear();
    if (Opening.valid()) Opening.get().Clear();
    Source.Clear();
    Wav.Close();
    if (mmcss) AvRevertMmThreadCharacteristics(mmcss);
  }
};
AudioOutput::AudioOutput(ALCdevice *device, bool hrtf,
                         const ComObject<AudioCaptureSource> &capture,
                         std::filesystem::path file,
                         const ComObject<OutputEndpointSource> &source)
    : _impl(std::make_unique<Impl>(*this, device, hrtf, capture,
                                   std::move(file), source)) {}
AudioOutput::~AudioOutput() { Stop(); }
HRESULT AudioOutput::Start() {
  std::promise<HRESULT> ready;
  auto future = ready.get_future();
  _impl->Thread = std::thread([this, ready = std::move(ready)]() mutable {
    const auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    HRESULT result = E_FAIL;
    try {
      result = _impl->Open();
    } catch (...) {
    }
    ready.set_value(result);
    if (SUCCEEDED(result)) _impl->Run();
    if (SUCCEEDED(com)) CoUninitialize();
  });
  return future.get();
}
void AudioOutput::Stop() {
  SetEvent(_impl->StopEvent);
  if (_impl->Thread.joinable()) _impl->Thread.join();
}
}  // namespace MGDF::core::audio::openal_audio
