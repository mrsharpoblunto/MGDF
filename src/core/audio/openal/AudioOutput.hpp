#pragma once

#include <AL/alc.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

#include "../MGDFAudioCapture.hpp"

namespace MGDF::core::audio::openal_audio {

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6103")) IOutputEndpoint
    : IUnknown {};

// The external device boundary. Tests replace only this interface.
class OutputEndpoint : public ComBase<IOutputEndpoint> {
 public:
  std::wstring Id;
  UINT32 Channels = 2;
  UINT32 BufferFrames = 0;
  HANDLE Event = nullptr;
  virtual HRESULT Start() = 0;
  virtual HRESULT Padding(UINT32 &frames) = 0;
  virtual HRESULT Buffer(UINT32 frames, float **data) = 0;
  virtual HRESULT Submit(UINT32 frames) = 0;
  virtual HRESULT Position(UINT64 &position, UINT64 &qpc,
                           UINT64 &frequency) = 0;
};

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6104"))
IOutputEndpointSource : IUnknown {};

class OutputEndpointSource : public ComBase<IOutputEndpointSource> {
 public:
  OutputEndpointSource();
  ~OutputEndpointSource();
  HANDLE Changed = nullptr;
  virtual ComObject<OutputEndpoint> Open() = 0;
};

class AudioOutput {
 public:
  AudioOutput(ALCdevice *device, bool hrtf,
              const ComObject<AudioCaptureSource> &capture,
              std::filesystem::path file,
              const ComObject<OutputEndpointSource> &source);
  ~AudioOutput();
  HRESULT Start();
  void Stop();
  std::atomic<UINT64> DeviceChanges{0};
  std::atomic<float> RenderMs{0};

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
}  // namespace MGDF::core::audio::openal_audio
