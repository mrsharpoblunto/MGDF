#pragma once

#include <MGDF/MGDF.h>
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
  HRESULT Open(const std::filesystem::path &path, uint32_t width,
               uint32_t height);
  // times are in 100 ns units and must increase
  HRESULT Write(const uint8_t *nv12, int64_t time, int64_t duration);
  HRESULT Close();

 private:
  ComObject<IMFSinkWriter> _writer;
  DWORD _stream = 0;
  uint32_t _width = 0;
  uint32_t _height = 0;
  bool _started = false;
};

struct CaptureState;
struct VideoState;
struct CaptureSlot;

// Records screenshots and videos from drawn frames. The request methods are
// safe on any thread; RT methods run on the render thread
class CaptureManager {
 public:
  explicit CaptureManager(std::filesystem::path folder);
  ~CaptureManager();

  HRESULT RecordScreenshot(const small *name, IMGDFCapture **capture);
  HRESULT RecordVideo(const small *name, IMGDFVideoRecording **recording);

  void RTSetDevice(ID3D12Device *device, ID3D12CommandQueue *queue,
                   ID3D12Fence *fence);
  // After the module's RTDraw, before present. Copies the back buffer when
  // drawn; the frame fence reaches frame.FrameOrdinal once the copy is done
  void RTCapture(const MGDFFrameInfo &frame, bool drawn,
                 CaptureEncoding encoding, float sdrWhiteNits);
  // Before the device goes away: delivers or drops every frame in flight
  void RTReleaseDevice();
  // Stops every recording and waits for the files
  void RTShutDown();

 private:
  void RTDeliver(uint64_t completedOrdinal);
  void RTEndVideos(bool all);
  CaptureSlot *RTFreeSlot(uint64_t bytes);
  void Work();

  std::filesystem::path _folder;
  std::mutex _requestMutex;
  std::vector<std::shared_ptr<CaptureState>> _screenshotRequests;
  std::vector<std::shared_ptr<VideoState>> _videoRequests;

  // render thread
  ComObject<ID3D12Device> _device;
  ComObject<ID3D12CommandQueue> _queue;
  ComObject<ID3D12Fence> _fence;
  std::vector<std::unique_ptr<CaptureSlot>> _slots;
  std::vector<std::shared_ptr<VideoState>> _videos;

  // screenshot encoding
  std::mutex _workMutex;
  std::condition_variable _workReady;
  std::deque<std::function<void()>> _work;
  bool _stopping = false;
  std::thread _worker;
};

}  // namespace core
}  // namespace MGDF
