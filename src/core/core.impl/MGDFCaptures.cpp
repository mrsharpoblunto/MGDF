#include "StdAfx.h"

#include "MGDFCaptures.hpp"

#include <DirectXPackedVector.h>
#include <codecapi.h>
#include <mfapi.h>
#include <wincodec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <execution>
#include <numeric>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFStringImpl.hpp"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "windowscodecs.lib")

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

namespace {

constexpr uint32_t kSlots = 8;
// frames waiting for a video encoder; each holds a readback slot
constexpr size_t kVideoQueue = 4;
constexpr uint32_t kVideoFrameRate = 60;
constexpr int64_t kTicksPerSecond = 10'000'000;

template <typename F>
void ForEachRow(uint32_t rows, F &&row) {
  std::vector<uint32_t> indices(rows);
  std::iota(indices.begin(), indices.end(), 0u);
  std::for_each(std::execution::par, indices.begin(), indices.end(), row);
}

class SRGBTable {
 public:
  SRGBTable() {
    for (size_t i = 0; i < _values.size(); ++i) {
      const double linear = double(i) / double(_values.size() - 1);
      const double encoded = linear <= 0.0031308
                                 ? 12.92 * linear
                                 : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
      _values[i] = uint8_t(std::lround(encoded * 255.0));
    }
  }
  uint8_t operator()(float linear) const {
    const auto index =
        std::clamp(int(linear * float(_values.size() - 1) + 0.5f), 0,
                   int(_values.size() - 1));
    return _values[index];
  }

 private:
  std::array<uint8_t, 4096> _values{};
};

const SRGBTable &SRGB() {
  static const SRGBTable table;
  return table;
}

// ST 2084 code value to nits
const std::array<float, 1024> &PQTable() {
  static const auto table = [] {
    std::array<float, 1024> values{};
    constexpr double m1 = 2610.0 / 16384.0, m2 = 2523.0 / 4096.0 * 128.0,
                     c1 = 3424.0 / 4096.0, c2 = 2413.0 / 4096.0 * 32.0,
                     c3 = 2392.0 / 4096.0 * 32.0;
    for (size_t i = 0; i < values.size(); ++i) {
      const double n = std::pow(double(i) / 1023.0, 1.0 / m2);
      values[i] =
          float(10000.0 *
                std::pow((std::max)(n - c1, 0.0) / (c2 - c3 * n), 1.0 / m1));
    }
    return values;
  }();
  return table;
}

}  // namespace

struct CaptureState {
  std::filesystem::path Path;
  std::atomic<bool> Complete = false;
  std::atomic<HRESULT> Result = E_PENDING;

  void Finish(HRESULT result) {
    Result = result;
    Complete = true;
  }
};

// a readback buffer that holds one frame
struct CaptureSlot {
  ComObject<ID3D12Resource> Buffer;
  ComObject<ID3D12CommandAllocator> Allocator;
  ComObject<ID3D12GraphicsCommandList> List;
  const uint8_t *Data = nullptr;
  uint64_t Bytes = 0;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
  uint64_t Ordinal = 0;
  bool InFlight = false;
  // workers that still read Data
  std::atomic<int> Users = 0;
  DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
  CaptureEncoding Encoding = CaptureEncoding::SDR;
  float SDRWhiteNits = 80.0f;
  int64_t Time = 0;
  std::vector<std::shared_ptr<CaptureState>> Screenshots;
  std::vector<std::shared_ptr<VideoState>> Videos;

  bool ToRGBA8(uint8_t *output) const {
    return ConvertToRGBA8(Data + Layout.Offset, Layout.Footprint.RowPitch,
                          Layout.Footprint.Width, Layout.Footprint.Height,
                          Format, Encoding, SDRWhiteNits, output);
  }
};

namespace {

// keeps a slot's pixels readable until the reader drops it
class SlotUse {
 public:
  explicit SlotUse(CaptureSlot &slot) : _slot(slot) { ++_slot.Users; }
  ~SlotUse() { --_slot.Users; }
  const CaptureSlot &Slot() const { return _slot; }

 private:
  CaptureSlot &_slot;
};

}  // namespace

struct VideoState : CaptureState {
  struct Frame {
    std::shared_ptr<SlotUse> Pixels;
    int64_t Time;
  };

  std::atomic<bool> StopRequested = false;
  // the encoder is open, so frames can be recorded
  std::atomic<bool> EncoderReady = false;
  std::atomic<uint64_t> Frames = 0;
  std::atomic<uint64_t> Dropped = 0;

  // render thread
  bool Started = false;
  uint32_t Width = 0;
  uint32_t Height = 0;
  int64_t LastTime = 0;
  uint32_t InFlight = 0;
  bool Ending = false;
  bool CloseSent = false;
  std::thread Worker;

  std::mutex Mutex;
  std::condition_variable Ready;
  std::deque<Frame> Queue;
  bool Closing = false;

  void Push(Frame frame) {
    {
      std::lock_guard lock(Mutex);
      if (Queue.size() < kVideoQueue) {
        Queue.push_back(std::move(frame));
        Ready.notify_one();
        return;
      }
    }
    ++Dropped;
  }

  void Close() {
    std::lock_guard lock(Mutex);
    Closing = true;
    Ready.notify_one();
  }

  // width and height must be even
  void Encode(uint32_t width, uint32_t height);
};

void VideoState::Encode(uint32_t width, uint32_t height) {
  const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  VideoEncoder encoder;
  // opening takes most of a second, so it must finish before the first frame
  // or the queue fills and frames are dropped
  HRESULT result = encoder.Open(Path, width, height);
  if (SUCCEEDED(result)) {
    EncoderReady = true;
  } else {
    StopRequested = true;
  }
  bool started = false;
  int64_t first = 0;
  std::vector<uint8_t> rgba, nv12, pending;
  int64_t pendingTime = 0;

  for (;;) {
    Frame frame;
    {
      std::unique_lock lock(Mutex);
      Ready.wait(lock, [this] { return !Queue.empty() || Closing; });
      if (Queue.empty()) break;
      frame = std::move(Queue.front());
      Queue.pop_front();
    }
    if (FAILED(result)) continue;
    const auto &slot = frame.Pixels->Slot();
    const uint32_t fullWidth = slot.Layout.Footprint.Width;
    if (!started) {
      started = true;
      first = frame.Time;
    }
    rgba.resize(size_t(fullWidth) * slot.Layout.Footprint.Height * 4);
    if (!slot.ToRGBA8(rgba.data())) {
      result = E_NOTIMPL;
      continue;
    }
    frame.Pixels.reset();
    nv12.resize(size_t(width) * height * 3 / 2);
    ConvertToNV12(rgba.data(), fullWidth * 4, width, height, nv12.data());
    const int64_t time =
        (frame.Time - first) * kTicksPerSecond / frequency.QuadPart;
    if (!pending.empty()) {
      result = encoder.Write(pending.data(), pendingTime, time - pendingTime);
      if (SUCCEEDED(result)) ++Frames;
    }
    pending.swap(nv12);
    pendingTime = time;
  }
  if (SUCCEEDED(result) && !pending.empty()) {
    result = encoder.Write(pending.data(), pendingTime,
                           kTicksPerSecond / kVideoFrameRate);
    if (SUCCEEDED(result)) ++Frames;
  }
  const HRESULT closed = encoder.Close();
  if (Frames == 0) {
    // stopped before a frame was recorded: there is no file
    std::error_code error;
    std::filesystem::remove(Path, error);
    if (SUCCEEDED(result)) result = HRESULT_FROM_WIN32(ERROR_NO_DATA);
  } else if (SUCCEEDED(result)) {
    result = closed;
  }
  Finish(result);
  if (SUCCEEDED(com)) CoUninitialize();
}

namespace {

class Capture final : public ComBase<IMGDFCapture> {
 public:
  explicit Capture(std::shared_ptr<CaptureState> state)
      : _state(std::move(state)) {}
  BOOL __stdcall IsComplete() final { return _state->Complete; }
  HRESULT __stdcall GetResult() final { return _state->Result; }
  HRESULT __stdcall GetPath(wchar_t *path, UINT64 *length) final {
    return StringWriter::Write(_state->Path.wstring(), path, length);
  }

 private:
  std::shared_ptr<CaptureState> _state;
};

class VideoRecording final : public ComBase<IMGDFVideoRecording, IMGDFCapture> {
 public:
  explicit VideoRecording(std::shared_ptr<VideoState> state)
      : _state(std::move(state)) {}
  ~VideoRecording() { _state->StopRequested = true; }
  BOOL __stdcall IsComplete() final { return _state->Complete; }
  HRESULT __stdcall GetResult() final { return _state->Result; }
  HRESULT __stdcall GetPath(wchar_t *path, UINT64 *length) final {
    return StringWriter::Write(_state->Path.wstring(), path, length);
  }
  void __stdcall Stop() final { _state->StopRequested = true; }
  BOOL __stdcall IsRecording() final {
    return _state->EncoderReady && !_state->StopRequested && !_state->Complete;
  }
  UINT64 __stdcall GetFrameCount() final { return _state->Frames; }
  UINT64 __stdcall GetDroppedFrameCount() final { return _state->Dropped; }

 private:
  std::shared_ptr<VideoState> _state;
};

void Transition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
                D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = resource,
                     .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                     .StateBefore = before,
                     .StateAfter = after}};
  list->ResourceBarrier(1, &barrier);
}

}  // namespace

bool ConvertToRGBA8(const uint8_t *source, uint32_t rowPitch, uint32_t width,
                    uint32_t height, DXGI_FORMAT format,
                    CaptureEncoding encoding, float sdrWhiteNits,
                    uint8_t *output) {
  const auto &srgb = SRGB();
  const auto rows = [&](auto &&pixel) {
    ForEachRow(height, [&](uint32_t y) {
      const uint8_t *in = source + size_t(y) * rowPitch;
      uint8_t *out = output + size_t(y) * width * 4;
      for (uint32_t x = 0; x < width; ++x) {
        pixel(in, x, out + size_t(x) * 4);
        out[size_t(x) * 4 + 3] = 255;
      }
    });
  };
  switch (format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      rows([](const uint8_t *in, uint32_t x, uint8_t *out) {
        std::copy_n(in + size_t(x) * 4, 3, out);
      });
      return true;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      rows([](const uint8_t *in, uint32_t x, uint8_t *out) {
        const uint8_t *p = in + size_t(x) * 4;
        out[0] = p[2];
        out[1] = p[1];
        out[2] = p[0];
      });
      return true;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
      if (encoding == CaptureEncoding::HDR10) {
        const auto &pq = PQTable();
        rows([&](const uint8_t *in, uint32_t x, uint8_t *out) {
          const uint32_t v = reinterpret_cast<const uint32_t *>(in)[x];
          const float r = pq[v & 1023] / sdrWhiteNits;
          const float g = pq[(v >> 10) & 1023] / sdrWhiteNits;
          const float b = pq[(v >> 20) & 1023] / sdrWhiteNits;
          // Rec.2020 to Rec.709 primaries
          out[0] = srgb(1.6605f * r - 0.5876f * g - 0.0728f * b);
          out[1] = srgb(-0.1246f * r + 1.1329f * g - 0.0083f * b);
          out[2] = srgb(-0.0182f * r - 0.1006f * g + 1.1187f * b);
        });
      } else {
        rows([](const uint8_t *in, uint32_t x, uint8_t *out) {
          const uint32_t v = reinterpret_cast<const uint32_t *>(in)[x];
          out[0] = uint8_t((v & 1023) >> 2);
          out[1] = uint8_t(((v >> 10) & 1023) >> 2);
          out[2] = uint8_t(((v >> 20) & 1023) >> 2);
        });
      }
      return true;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT: {
      const float scale =
          encoding == CaptureEncoding::ScRGB ? 80.0f / sdrWhiteNits : 1.0f;
      rows([&](const uint8_t *in, uint32_t x, uint8_t *out) {
        const auto *p = reinterpret_cast<const uint16_t *>(in) + size_t(x) * 4;
        for (int c = 0; c < 3; ++c) {
          out[c] =
              srgb(DirectX::PackedVector::XMConvertHalfToFloat(p[c]) * scale);
        }
      });
      return true;
    }
    default:
      return false;
  }
}

void ConvertToNV12(const uint8_t *rgba, uint32_t rgbaRowPitch, uint32_t width,
                   uint32_t height, uint8_t *output) {
  uint8_t *luma = output;
  uint8_t *chroma = output + size_t(width) * height;
  const auto y = [](float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  };
  ForEachRow(height / 2, [&](uint32_t pair) {
    const uint8_t *top = rgba + size_t(pair) * 2 * rgbaRowPitch;
    const uint8_t *bottom = top + rgbaRowPitch;
    uint8_t *lumaTop = luma + size_t(pair) * 2 * width;
    uint8_t *lumaBottom = lumaTop + width;
    uint8_t *uv = chroma + size_t(pair) * width;
    for (uint32_t x = 0; x < width; x += 2) {
      float r = 0, g = 0, b = 0;
      const uint8_t *pixels[] = {top + x * 4, top + x * 4 + 4, bottom + x * 4,
                                 bottom + x * 4 + 4};
      uint8_t *lumas[] = {lumaTop + x, lumaTop + x + 1, lumaBottom + x,
                          lumaBottom + x + 1};
      for (int i = 0; i < 4; ++i) {
        const float pr = pixels[i][0] / 255.0f, pg = pixels[i][1] / 255.0f,
                    pb = pixels[i][2] / 255.0f;
        *lumas[i] = uint8_t(std::lround(16.0f + 219.0f * y(pr, pg, pb)));
        r += pr * 0.25f;
        g += pg * 0.25f;
        b += pb * 0.25f;
      }
      const float average = y(r, g, b);
      uv[x] = uint8_t(std::lround(128.0f + 224.0f * (b - average) / 1.8556f));
      uv[x + 1] =
          uint8_t(std::lround(128.0f + 224.0f * (r - average) / 1.5748f));
    }
  });
}

std::optional<std::filesystem::path> CapturePath(
    const std::filesystem::path &folder, std::string_view name,
    std::wstring_view extension) {
  if (name.empty() || name.size() > 1024) return {};
  const int size = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), int(name.size()), nullptr, 0);
  if (size <= 0) return {};
  std::wstring wide(size_t(size), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(),
                      int(name.size()), wide.data(), size);
  std::filesystem::path result = folder;
  size_t start = 0;
  for (;;) {
    const size_t end = wide.find_first_of(L"/\\", start);
    const std::wstring part = wide.substr(start, end - start);
    if (part.empty() || part == L"." || part == L".." || part.back() == L'.' ||
        part.back() == L' ')
      return {};
    for (const wchar_t c : part) {
      if (c < 32 ||
          std::wstring_view(L"<>:\"|?*").find(c) != std::wstring_view::npos)
        return {};
    }
    result /= part;
    if (end == std::wstring::npos) break;
    start = end + 1;
  }
  const auto file = result.filename().native();
  if (file.size() <= extension.size() ||
      _wcsicmp(file.c_str() + file.size() - extension.size(),
               std::wstring(extension).c_str()) != 0) {
    result += extension;
  }
  return result.make_preferred();
}

HRESULT WritePNG(const std::filesystem::path &path, const uint8_t *rgba,
                 uint32_t width, uint32_t height) {
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  ComObject<IWICImagingFactory> factory;
  HRESULT result =
      CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                       IID_PPV_ARGS(factory.Assign()));
  ComObject<IWICStream> stream;
  if (SUCCEEDED(result)) result = factory->CreateStream(stream.Assign());
  if (SUCCEEDED(result))
    result = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
  ComObject<IWICBitmapEncoder> encoder;
  if (SUCCEEDED(result))
    result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr,
                                    encoder.Assign());
  if (SUCCEEDED(result))
    result = encoder->Initialize(stream, WICBitmapEncoderNoCache);
  ComObject<IWICBitmapFrameEncode> frame;
  if (SUCCEEDED(result))
    result = encoder->CreateNewFrame(frame.Assign(), nullptr);
  if (SUCCEEDED(result)) result = frame->Initialize(nullptr);
  if (SUCCEEDED(result)) result = frame->SetSize(width, height);
  WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
  if (SUCCEEDED(result)) result = frame->SetPixelFormat(&format);
  ComObject<IWICBitmap> bitmap;
  if (SUCCEEDED(result))
    result = factory->CreateBitmapFromMemory(
        width, height, GUID_WICPixelFormat32bppRGBA, width * 4,
        width * height * 4, const_cast<BYTE *>(rgba), bitmap.Assign());
  // WriteSource converts to the frame's pixel format
  if (SUCCEEDED(result)) result = frame->WriteSource(bitmap, nullptr);
  if (SUCCEEDED(result)) result = frame->Commit();
  if (SUCCEEDED(result)) result = encoder->Commit();
  return result;
}

VideoEncoder::~VideoEncoder() { Close(); }

HRESULT VideoEncoder::Open(const std::filesystem::path &path, uint32_t width,
                           uint32_t height) {
  _width = width;
  _height = height;
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  HRESULT result = MFStartup(MF_VERSION, MFSTARTUP_LITE);
  if (FAILED(result)) return result;
  _started = true;
  ComObject<IMFAttributes> attributes;
  result = MFCreateAttributes(attributes.Assign(), 2);
  if (SUCCEEDED(result))
    result =
        attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  if (SUCCEEDED(result))
    result = attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
  ComObject<IMFSinkWriter> writer;
  if (SUCCEEDED(result))
    result = MFCreateSinkWriterFromURL(path.c_str(), nullptr, attributes,
                                       writer.Assign());
  const auto describe = [&](IMFMediaType *type, const GUID &subtype) {
    HRESULT r = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(r)) r = type->SetGUID(MF_MT_SUBTYPE, subtype);
    if (SUCCEEDED(r))
      r = type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(r))
      r = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(r))
      r = MFSetAttributeRatio(type, MF_MT_FRAME_RATE, kVideoFrameRate, 1);
    if (SUCCEEDED(r))
      r = MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    return r;
  };
  ComObject<IMFMediaType> output;
  if (SUCCEEDED(result)) result = MFCreateMediaType(output.Assign());
  if (SUCCEEDED(result)) result = describe(output, MFVideoFormat_H264);
  // about 0.15 bits per pixel at the nominal frame rate
  const auto bitrate = uint32_t(std::clamp<uint64_t>(
      uint64_t(width) * height * kVideoFrameRate * 15 / 100, 2'000'000,
      200'000'000));
  if (SUCCEEDED(result)) result = output->SetUINT32(MF_MT_AVG_BITRATE, bitrate);
  if (SUCCEEDED(result))
    result = output->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High);
  if (SUCCEEDED(result)) result = writer->AddStream(output, &_stream);
  ComObject<IMFMediaType> input;
  if (SUCCEEDED(result)) result = MFCreateMediaType(input.Assign());
  if (SUCCEEDED(result)) result = describe(input, MFVideoFormat_NV12);
  if (SUCCEEDED(result))
    result = writer->SetInputMediaType(_stream, input, nullptr);
  if (SUCCEEDED(result)) result = writer->BeginWriting();
  if (SUCCEEDED(result)) _writer = writer;
  return result;
}

HRESULT VideoEncoder::Write(const uint8_t *nv12, int64_t time,
                            int64_t duration) {
  if (!_writer) return E_UNEXPECTED;
  const DWORD bytes = _width * _height * 3 / 2;
  ComObject<IMFMediaBuffer> buffer;
  HRESULT result = MFCreateMemoryBuffer(bytes, buffer.Assign());
  BYTE *data = nullptr;
  if (SUCCEEDED(result)) result = buffer->Lock(&data, nullptr, nullptr);
  if (SUCCEEDED(result)) {
    std::copy_n(nv12, bytes, data);
    buffer->Unlock();
    result = buffer->SetCurrentLength(bytes);
  }
  ComObject<IMFSample> sample;
  if (SUCCEEDED(result)) result = MFCreateSample(sample.Assign());
  if (SUCCEEDED(result)) result = sample->AddBuffer(buffer);
  if (SUCCEEDED(result)) result = sample->SetSampleTime(time);
  if (SUCCEEDED(result)) result = sample->SetSampleDuration(duration);
  if (SUCCEEDED(result)) result = _writer->WriteSample(_stream, sample);
  return result;
}

HRESULT VideoEncoder::Close() {
  HRESULT result = S_OK;
  if (_writer) {
    result = _writer->Finalize();
    _writer.Clear();
  }
  if (_started) {
    MFShutdown();
    _started = false;
  }
  return result;
}

CaptureManager::CaptureManager(std::filesystem::path folder)
    : _folder(std::move(folder)), _worker([this] { Work(); }) {}

CaptureManager::~CaptureManager() {
  {
    std::lock_guard lock(_workMutex);
    _stopping = true;
    _workReady.notify_one();
  }
  _worker.join();
  for (auto &video : _videos) {
    if (video->Worker.joinable()) {
      video->Close();
      video->Worker.join();
    }
  }
}

void CaptureManager::Work() {
  const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  for (;;) {
    std::function<void()> work;
    {
      std::unique_lock lock(_workMutex);
      _workReady.wait(lock, [this] { return !_work.empty() || _stopping; });
      if (_work.empty()) break;
      work = std::move(_work.front());
      _work.pop_front();
    }
    work();
  }
  if (SUCCEEDED(com)) CoUninitialize();
}

HRESULT CaptureManager::RecordScreenshot(const small *name,
                                         IMGDFCapture **capture) {
  if (!name || !capture) return E_INVALIDARG;
  const auto path = CapturePath(_folder, name, L".png");
  if (!path) return E_INVALIDARG;
  auto state = std::make_shared<CaptureState>();
  state->Path = *path;
  {
    std::lock_guard lock(_requestMutex);
    _screenshotRequests.push_back(state);
  }
  *capture = new Capture(std::move(state));
  return S_OK;
}

HRESULT CaptureManager::RecordVideo(const small *name,
                                    IMGDFVideoRecording **recording) {
  if (!name || !recording) return E_INVALIDARG;
  const auto path = CapturePath(_folder, name, L".mp4");
  if (!path) return E_INVALIDARG;
  auto state = std::make_shared<VideoState>();
  state->Path = *path;
  {
    std::lock_guard lock(_requestMutex);
    _videoRequests.push_back(state);
  }
  *recording = new VideoRecording(std::move(state));
  return S_OK;
}

void CaptureManager::RTSetDevice(ID3D12Device *device,
                                 ID3D12CommandQueue *queue,
                                 ID3D12Fence *fence) {
  _device = MakeComFromPtr<ID3D12Device>(device);
  _queue = MakeComFromPtr<ID3D12CommandQueue>(queue);
  _fence = MakeComFromPtr<ID3D12Fence>(fence);
}

void CaptureManager::RTDeliver(uint64_t completedOrdinal) {
  for (auto &slot : _slots) {
    if (!slot->InFlight || slot->Ordinal > completedOrdinal) continue;
    slot->InFlight = false;
    for (auto &screenshot : slot->Screenshots) {
      auto pixels = std::make_shared<SlotUse>(*slot);
      std::lock_guard lock(_workMutex);
      _work.push_back([pixels, screenshot] {
        const auto &source = pixels->Slot();
        const uint32_t width = source.Layout.Footprint.Width;
        const uint32_t height = source.Layout.Footprint.Height;
        std::vector<uint8_t> rgba(size_t(width) * height * 4);
        screenshot->Finish(
            source.ToRGBA8(rgba.data())
                ? WritePNG(screenshot->Path, rgba.data(), width, height)
                : E_NOTIMPL);
      });
      _workReady.notify_one();
    }
    for (auto &video : slot->Videos) {
      --video->InFlight;
      video->Push({std::make_shared<SlotUse>(*slot), slot->Time});
    }
    slot->Screenshots.clear();
    slot->Videos.clear();
  }
}

void CaptureManager::RTEndVideos(bool all) {
  for (auto it = _videos.begin(); it != _videos.end();) {
    auto &video = **it;
    if (all) video.Ending = true;
    if (video.Ending && video.InFlight == 0 && !video.CloseSent) {
      video.CloseSent = true;
      video.Close();
    }
    if (video.CloseSent && (all || video.Complete)) {
      video.Worker.join();
      it = _videos.erase(it);
      continue;
    }
    ++it;
  }
}

CaptureSlot *CaptureManager::RTFreeSlot(uint64_t bytes) {
  CaptureSlot *free = nullptr;
  for (auto &slot : _slots) {
    if (slot->InFlight || slot->Users != 0) continue;
    if (slot->Bytes >= bytes) return slot.get();
    free = slot.get();
  }
  if (!free && _slots.size() < kSlots) {
    _slots.push_back(std::make_unique<CaptureSlot>());
    free = _slots.back().get();
    if (FAILED(_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(free->Allocator.Assign()))) ||
        FAILED(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          free->Allocator, nullptr,
                                          IID_PPV_ARGS(free->List.Assign()))) ||
        FAILED(free->List->Close())) {
      _slots.pop_back();
      return nullptr;
    }
  }
  if (!free) return nullptr;
  // grow the buffer of a free slot to fit the frame
  free->Buffer.Clear();
  free->Data = nullptr;
  free->Bytes = 0;
  const D3D12_HEAP_PROPERTIES heap{.Type = D3D12_HEAP_TYPE_READBACK};
  const D3D12_RESOURCE_DESC desc{.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                                 .Width = bytes,
                                 .Height = 1,
                                 .DepthOrArraySize = 1,
                                 .MipLevels = 1,
                                 .SampleDesc = {.Count = 1},
                                 .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
  void *data = nullptr;
  if (FAILED(_device->CreateCommittedResource(
          &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr, IID_PPV_ARGS(free->Buffer.Assign()))) ||
      FAILED(free->Buffer->Map(0, nullptr, &data))) {
    free->Buffer.Clear();
    return nullptr;
  }
  // the buffer stays mapped; it is read only after the frame fence passes
  free->Data = static_cast<const uint8_t *>(data);
  free->Bytes = bytes;
  return free;
}

void CaptureManager::RTCapture(const MGDFFrameInfo &frame, bool drawn,
                               CaptureEncoding encoding, float sdrWhiteNits) {
  if (!_device) return;
  RTDeliver(frame.CompletedOrdinal);
  const auto &desc = frame.BackBufferDesc;
  const auto width = uint32_t(desc.Width);
  const auto height = desc.Height;
  std::vector<std::shared_ptr<CaptureState>> screenshots;
  {
    std::lock_guard lock(_requestMutex);
    if (drawn) screenshots.swap(_screenshotRequests);
    for (auto &video : _videoRequests) {
      video->Width = width;
      video->Height = height;
      video->Worker = std::thread(
          [video, width, height] { video->Encode(width & ~1u, height & ~1u); });
      _videos.push_back(std::move(video));
    }
    _videoRequests.clear();
  }

  LARGE_INTEGER now, frequency;
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&frequency);
  // a little early still counts, so a 60 Hz display records every frame
  const int64_t interval =
      frequency.QuadPart / kVideoFrameRate - frequency.QuadPart / 1000;
  std::vector<std::shared_ptr<VideoState>> videos;
  for (auto &video : _videos) {
    if (video->Ending) continue;
    if (video->StopRequested) {
      video->Ending = true;
    } else if (video->Width != width || video->Height != height) {
      LOG("Video recording " << video->Path.string()
                             << " ended at a back buffer size change",
          MGDF_LOG_MEDIUM);
      video->StopRequested = true;
      video->Ending = true;
    } else if (drawn && video->EncoderReady &&
               (!video->Started ||
                now.QuadPart - video->LastTime >= interval)) {
      videos.push_back(video);
    }
  }

  if (!screenshots.empty() || !videos.empty()) {
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
    UINT64 bytes = 0;
    _device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr,
                                   &bytes);
    CaptureSlot *slot = RTFreeSlot(bytes);
    if (!slot) {
      for (auto &video : videos) ++video->Dropped;
      std::lock_guard lock(_requestMutex);
      _screenshotRequests.insert(_screenshotRequests.begin(),
                                 screenshots.begin(), screenshots.end());
    } else if (FAILED(slot->Allocator->Reset()) ||
               FAILED(slot->List->Reset(slot->Allocator, nullptr))) {
      for (auto &video : videos) ++video->Dropped;
      for (auto &screenshot : screenshots) screenshot->Finish(E_FAIL);
    } else {
      auto *list = slot->List.Get();
      Transition(list, frame.BackBuffer, D3D12_RESOURCE_STATE_PRESENT,
                 D3D12_RESOURCE_STATE_COPY_SOURCE);
      const D3D12_TEXTURE_COPY_LOCATION target{
          .pResource = slot->Buffer,
          .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
          .PlacedFootprint = layout};
      const D3D12_TEXTURE_COPY_LOCATION source{
          .pResource = frame.BackBuffer,
          .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
          .SubresourceIndex = 0};
      list->CopyTextureRegion(&target, 0, 0, 0, &source, nullptr);
      Transition(list, frame.BackBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE,
                 D3D12_RESOURCE_STATE_PRESENT);
      if (FAILED(list->Close())) {
        for (auto &video : videos) ++video->Dropped;
        for (auto &screenshot : screenshots) screenshot->Finish(E_FAIL);
      } else {
        ID3D12CommandList *lists[] = {list};
        _queue->ExecuteCommandLists(1, lists);
        slot->Layout = layout;
        slot->Ordinal = frame.FrameOrdinal;
        slot->InFlight = true;
        slot->Format = desc.Format;
        slot->Encoding = encoding;
        slot->SDRWhiteNits = sdrWhiteNits;
        slot->Time = now.QuadPart;
        slot->Screenshots = std::move(screenshots);
        slot->Videos = videos;
        for (auto &video : videos) {
          video->Started = true;
          video->LastTime = now.QuadPart;
          ++video->InFlight;
        }
      }
    }
  }
  RTEndVideos(false);
}

void CaptureManager::RTReleaseDevice() {
  if (!_device) return;
  uint64_t last = 0;
  for (auto &slot : _slots) {
    if (slot->InFlight) last = (std::max)(last, slot->Ordinal);
  }
  if (last > _fence->GetCompletedValue()) {
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (event) {
      if (SUCCEEDED(_fence->SetEventOnCompletion(last, event)))
        WaitForSingleObject(event, 5000);
      CloseHandle(event);
    }
  }
  const uint64_t completed = _fence->GetCompletedValue();
  RTDeliver(completed);
  // frames the GPU did not finish are lost
  for (auto &slot : _slots) {
    if (!slot->InFlight) continue;
    slot->InFlight = false;
    for (auto &video : slot->Videos) {
      --video->InFlight;
      ++video->Dropped;
    }
    for (auto &screenshot : slot->Screenshots) {
      std::lock_guard lock(_requestMutex);
      _screenshotRequests.push_back(screenshot);
    }
    slot->Screenshots.clear();
    slot->Videos.clear();
  }
  // workers read the mapped buffers until they finish with them
  for (auto &slot : _slots) {
    while (slot->Users != 0) Sleep(1);
  }
  _slots.clear();
  _fence.Clear();
  _queue.Clear();
  _device.Clear();
}

void CaptureManager::RTShutDown() {
  RTReleaseDevice();
  {
    std::lock_guard lock(_requestMutex);
    for (auto &screenshot : _screenshotRequests) screenshot->Finish(E_ABORT);
    _screenshotRequests.clear();
    for (auto &video : _videoRequests) video->Finish(E_ABORT);
    _videoRequests.clear();
  }
  RTEndVideos(true);
}

}  // namespace core
}  // namespace MGDF
