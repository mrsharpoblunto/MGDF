#pragma once

#include <MGDF/MGDF.h>
#include <d3d11.h>
#include <d3d12.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

#include "../audio/MGDFAudioCapture.hpp"

namespace MGDF {
namespace core {

// How the back buffer encodes color
enum class CaptureEncoding {
  SDR,
  // linear Rec.709, 1.0 is 80 nits
  ScRGB,
  // PQ, Rec.2020
  HDR10,
};

// Media Foundation is delay loaded: Windows N and KN editions do not have it
// unless the Media Feature Pack is installed
bool MediaFoundationInstalled();

// Converts back buffer rows to tightly packed 8-bit sRGB RGBA with opaque
// alpha. HDR maps sdrWhiteNits to white and clips above it, as an SDR display
// would. Returns false for an unsupported format
bool ConvertToRGBA8(const uint8_t *source, uint32_t rowPitch, uint32_t width,
                    uint32_t height, DXGI_FORMAT format,
                    CaptureEncoding encoding, float sdrWhiteNits,
                    uint8_t *output);

// Rec.709 limited range NV12. width and height must be even
void ConvertToNV12(const uint8_t *rgba, uint32_t rgbaRowPitch, uint32_t width,
                   uint32_t height, uint8_t *output);

// The path for a capture name in folder, or nothing for an invalid name. The
// extension is added unless the name already ends with it
std::optional<std::filesystem::path> CapturePath(
    const std::filesystem::path &folder, std::string_view name,
    std::wstring_view extension);

HRESULT WritePNG(const std::filesystem::path &path, const uint8_t *rgba,
                 uint32_t width, uint32_t height);

// Writes NV12 frames to an H.264 MP4. Call from one thread
class VideoEncoder {
 public:
  ~VideoEncoder();
  // width and height must be even
  // device is optional; without one Media Foundation creates a device for
  // the encoder, which takes most of a second
  HRESULT Open(const std::filesystem::path &path, uint32_t width,
               uint32_t height, IMFDXGIDeviceManager *device = nullptr,
               bool audio = false);
  // times are in 100 ns units and must increase
  HRESULT Write(const uint8_t *nv12, int64_t time, int64_t duration);
  HRESULT WriteAudio(const int16_t *stereo, UINT32 frames, int64_t time);
  HRESULT Close();

 private:
  ComObject<IMFSinkWriter> _writer;
  DWORD _stream = 0, _audioStream = 0;
  bool _audio = false;
  uint32_t _width = 0;
  uint32_t _height = 0;
  bool _started = false;
};

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6112"))
ICaptureWorkerControl : IUnknown {};
// Optional worker signals for bounded integration tests.
class CaptureWorkerControl : public ComBase<ICaptureWorkerControl> {
 public:
  CaptureWorkerControl()
      : Ready(CreateEventW(nullptr, TRUE, FALSE, nullptr)),
        Resume(CreateEventW(nullptr, TRUE, TRUE, nullptr)),
        Progress(CreateEventW(nullptr, FALSE, FALSE, nullptr)) {}
  ~CaptureWorkerControl() {
    CloseHandle(Ready);
    CloseHandle(Resume);
    CloseHandle(Progress);
  }
  HANDLE Ready, Resume, Progress;
};

struct CaptureState;
struct VideoState;
struct CaptureSlot;

// Records screenshots and videos from drawn frames. The recorder methods are
// safe on any thread; RT methods run on the render thread
class CaptureManager : public ComBase<IMGDFRecorder> {
 public:
  // deviceLost: the host is in a device reset
  CaptureManager(std::filesystem::path folder, bool deviceLost,
                 const ComObject<audio::AudioCaptureSource> &audio = {},
                 const ComObject<CaptureWorkerControl> &control = {});
  ~CaptureManager();

  HRESULT __stdcall TakeScreenshot(const small *name,
                                   IMGDFCapture **capture) final;
  HRESULT __stdcall StartVideo(const small *name,
                               IMGDFVideoRecording **recording) final;

  bool RTHasDevice() const { return _device; }
  void RTSetDevice(ID3D12Device *device, ID3D12CommandQueue *queue,
                   ID3D12Fence *fence);
  // After the module's RTDraw, before present. Copies the back buffer when
  // drawn; the frame fence reaches frame.FrameOrdinal once the copy is done
  void RTCapture(const MGDFFrameInfo &frame, bool drawn,
                 CaptureEncoding encoding, float sdrWhiteNits,
                 int64_t qpcTime = 0);
  // Before a device reset: captures in progress fail, and new captures fail
  // until RTSetDevice
  void RTReleaseDevice();
  // Stops every recording and waits for the files
  void RTShutDown();

 private:
  void RTDeliver(uint64_t completedOrdinal);
  // delivers finished frames and fails screenshots still on the GPU with lost
  void RTDropDevice(HRESULT lost);
  void RTEndVideos(bool all);
  CaptureSlot *RTFreeSlot(uint64_t bytes);
  void Work();
  // the shared encoder device on the host's adapter, created on first use
  // and again after it is removed; null on a software adapter
  ComObject<IMFDXGIDeviceManager> EncoderDevice();

  std::filesystem::path _folder;
  ComObject<audio::AudioCaptureSource> _audio;
  ComObject<CaptureWorkerControl> _control;

  std::mutex _encoderDeviceMutex;
  LUID _adapter{};
  bool _adapterKnown = false;
  bool _encoderDeviceTried = false;
  LUID _encoderAdapter{};
  ComObject<ID3D11Device> _encoderD3DDevice;
  ComObject<IMFDXGIDeviceManager> _encoderDevice;
  std::mutex _requestMutex;
  std::vector<ComObject<CaptureState>> _screenshotRequests;
  std::vector<ComObject<VideoState>> _videoRequests;
  ComObject<IUnknown> _activeVideo;
  bool _shutDown = false;
  bool _deviceLost = false;

  // render thread
  ComObject<ID3D12Device> _device;
  ComObject<ID3D12CommandQueue> _queue;
  ComObject<ID3D12Fence> _fence;
  std::vector<std::unique_ptr<CaptureSlot>> _slots;
  std::vector<ComObject<VideoState>> _videos;

  // screenshot encoding
  std::mutex _workMutex;
  std::condition_variable _workReady;
  std::deque<std::function<void()>> _work;
  bool _stopping = false;
  std::thread _worker;
};

}  // namespace core
}  // namespace MGDF
