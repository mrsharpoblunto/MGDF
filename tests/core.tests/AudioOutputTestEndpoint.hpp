#pragma once
#include "../../src/core/audio/openal/AudioOutput.hpp"

namespace MGDF::core::audio::openal_audio {
struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6120")) ITestOutputState
    : IUnknown {};
class TestOutputState : public ComBase<ITestOutputState> {
 public:
  TestOutputState()
      : Opened(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
        Resume(CreateEventW(nullptr, TRUE, TRUE, nullptr)),
        Rendering(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
        RenderResume(CreateEventW(nullptr, TRUE, TRUE, nullptr)) {}
  ~TestOutputState() {
    CloseHandle(Opened);
    CloseHandle(Resume);
    CloseHandle(Rendering);
    CloseHandle(RenderResume);
  }
  std::atomic<UINT32> Channels{2};
  HANDLE Opened, Resume, Rendering, RenderResume;
};
class TestOutputEndpoint final : public OutputEndpoint {
 public:
  TestOutputEndpoint(UINT32 channels, const ComObject<TestOutputState> &state)
      : _state(state) {
    Channels = channels;
    BufferFrames = 240;
    Event = CreateWaitableTimerW(nullptr, FALSE, nullptr);
  }
  ~TestOutputEndpoint() {
    CancelWaitableTimer(Event);
    CloseHandle(Event);
  }
  HRESULT Start() final {
    LARGE_INTEGER due;
    due.QuadPart = -50000;
    return SetWaitableTimer(Event, &due, 5, nullptr, nullptr, FALSE) ? S_OK
                                                                     : E_FAIL;
  }
  HRESULT Padding(UINT32 &frames) final {
    SetEvent(_state->Rendering);
    if (WaitForSingleObject(_state->RenderResume, 10000) != WAIT_OBJECT_0)
      return E_FAIL;
    frames = 0;
    return S_OK;
  }
  HRESULT Buffer(UINT32 frames, float **data) final {
    if (frames > BufferFrames) return E_INVALIDARG;
    *data = _samples.data();
    return S_OK;
  }
  HRESULT Submit(UINT32 frames) final {
    _frames += frames;
    return S_OK;
  }
  HRESULT Position(UINT64 &position, UINT64 &qpc, UINT64 &frequency) final {
    LARGE_INTEGER now, rate;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&rate);
    position = _frames;
    frequency = 48000;
    qpc = static_cast<UINT64>(double(now.QuadPart) * 1e7 / rate.QuadPart);
    return S_OK;
  }

 private:
  ComObject<TestOutputState> _state;
  std::array<float, 240 * 8> _samples{};
  UINT64 _frames = 0;
};
class TestOutputSource final : public OutputEndpointSource {
 public:
  ComObject<TestOutputState> State = MakeCom<TestOutputState>();
  ComObject<OutputEndpoint> Open() final {
    SetEvent(State->Opened);
    if (WaitForSingleObject(State->Resume, 10000) != WAIT_OBJECT_0) return {};
    const auto channels = State->Channels.load();
    if (!channels) return {};
    auto endpoint = MakeCom<TestOutputEndpoint>(channels, State);
    return MakeComFromPtr<OutputEndpoint>(endpoint.Get());
  }
  void Change(UINT32 channels) {
    State->Channels.store(channels);
    SetEvent(Changed);
  }
};
}  // namespace MGDF::core::audio::openal_audio
