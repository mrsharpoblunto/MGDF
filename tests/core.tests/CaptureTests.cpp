#include "stdafx.h"

#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wincodec.h>

#include <MGDF/ComObject.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <thread>

#include "../../src/core/audio/openal/OpenALSoundManagerComponentImpl.hpp"
#include "../../src/core/core.impl/MGDFCaptures.hpp"
#include "AudioOutputTestEndpoint.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {

std::array<uint8_t, 4> Convert(DXGI_FORMAT format, CaptureEncoding encoding,
                               float white, const void *pixel, uint32_t pitch) {
  std::array<uint8_t, 4> rgba{};
  EXPECT_TRUE(ConvertToRGBA8(static_cast<const uint8_t *>(pixel), pitch, 1, 1,
                             format, encoding, white, rgba.data()));
  return rgba;
}

uint32_t PackR10G10B10(uint32_t r, uint32_t g, uint32_t b) {
  return r | (g << 10) | (b << 20) | (3u << 30);
}

// ST 2084 reference points: 100 nits is code 520, 1000 nits is code 769
constexpr uint32_t kPQ100 = 520;
constexpr uint32_t kPQ1000 = 769;

std::array<uint16_t, 4> Half(float value) {
  const auto h = DirectX::PackedVector::XMConvertFloatToHalf(value);
  return {h, h, h, DirectX::PackedVector::XMConvertFloatToHalf(1.0f)};
}

std::filesystem::path TempFolder() {
  return std::filesystem::temp_directory_path() /
         ("mgdf_captures_" + std::to_string(GetCurrentProcessId()));
}

std::vector<uint8_t> ReadPNG(const std::filesystem::path &path, UINT &width,
                             UINT &height) {
  ComObject<IWICImagingFactory> factory;
  EXPECT_EQ(S_OK, CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(factory.Assign())));
  ComObject<IWICBitmapDecoder> decoder;
  EXPECT_EQ(S_OK, factory->CreateDecoderFromFilename(
                      path.c_str(), nullptr, GENERIC_READ,
                      WICDecodeMetadataCacheOnDemand, decoder.Assign()));
  ComObject<IWICBitmapFrameDecode> frame;
  EXPECT_EQ(S_OK, decoder->GetFrame(0, frame.Assign()));
  ComObject<IWICFormatConverter> converter;
  EXPECT_EQ(S_OK, factory->CreateFormatConverter(converter.Assign()));
  EXPECT_EQ(S_OK, converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0.0,
                                        WICBitmapPaletteTypeCustom));
  EXPECT_EQ(S_OK, frame->GetSize(&width, &height));
  std::vector<uint8_t> pixels(size_t(width) * height * 4);
  EXPECT_EQ(S_OK, converter->CopyPixels(nullptr, width * 4, UINT(pixels.size()),
                                        pixels.data()));
  return pixels;
}

// the luma of the first pixel of each frame, and the frame size
std::vector<uint8_t> ReadVideoLuma(const std::filesystem::path &path,
                                   UINT32 &width, UINT32 &height) {
  constexpr auto stream = DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
  std::vector<uint8_t> luma;
  ComObject<IMFSourceReader> reader;
  EXPECT_EQ(S_OK, MFCreateSourceReaderFromURL(path.c_str(), nullptr,
                                              reader.Assign()));
  if (!reader) return luma;
  ComObject<IMFMediaType> type;
  EXPECT_EQ(S_OK, reader->GetNativeMediaType(stream, 0, type.Assign()));
  EXPECT_EQ(S_OK, MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height));
  ComObject<IMFMediaType> nv12;
  EXPECT_EQ(S_OK, MFCreateMediaType(nv12.Assign()));
  EXPECT_EQ(S_OK, nv12->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  EXPECT_EQ(S_OK, nv12->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
  EXPECT_EQ(S_OK, reader->SetCurrentMediaType(stream, nullptr, nv12));
  for (;;) {
    DWORD flags = 0;
    ComObject<IMFSample> sample;
    EXPECT_EQ(S_OK, reader->ReadSample(stream, 0, nullptr, &flags, nullptr,
                                       sample.Assign()));
    if (sample) {
      ComObject<IMFMediaBuffer> buffer;
      EXPECT_EQ(S_OK, sample->ConvertToContiguousBuffer(buffer.Assign()));
      BYTE *data = nullptr;
      EXPECT_EQ(S_OK, buffer->Lock(&data, nullptr, nullptr));
      luma.push_back(data[0]);
      buffer->Unlock();
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
  }
  return luma;
}

// Rec.709 limited range luma of black; white is 235
constexpr uint8_t kBlackLuma = 16;

template <typename Capture>
bool WaitForCompletion(Capture *capture) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while (capture->GetResult() == E_PENDING) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

}  // namespace

TEST(CaptureTests, NamesStayInsideTheCaptureFolder) {
  const std::filesystem::path folder = L"C:/user/captures";
  EXPECT_EQ(folder / L"shot.png", CapturePath(folder, "shot", L".png"));
  EXPECT_EQ(folder / L"shot.PNG", CapturePath(folder, "shot.PNG", L".png"));
  EXPECT_EQ(folder / L"waves" / L"cliff.mp4",
            CapturePath(folder, "waves/cliff", L".mp4"));
  EXPECT_EQ(folder / L"a" / L"b.mp4", CapturePath(folder, "a\\b", L".mp4"));
  for (const char *name : {"", "../out", "a/../b", "/root", "C:/out", "a//b",
                           "a/", "shot.", "bad|name", "tab\tname"}) {
    EXPECT_FALSE(CapturePath(folder, name, L".png")) << name;
  }
}

TEST(CaptureTests, ConvertsBackBufferFormatsToSRGB) {
  const uint8_t bgra[] = {10, 20, 30, 0};
  EXPECT_EQ((std::array<uint8_t, 4>{30, 20, 10, 255}),
            Convert(DXGI_FORMAT_B8G8R8A8_UNORM, CaptureEncoding::SDR, 80.0f,
                    bgra, 4));

  // HDR10: the SDR white level is white, half of it is sRGB 0.5, more clips
  const uint32_t white = PackR10G10B10(kPQ100, kPQ100, kPQ100);
  const auto mapped = Convert(DXGI_FORMAT_R10G10B10A2_UNORM,
                              CaptureEncoding::HDR10, 100.0f, &white, 4);
  EXPECT_NEAR(mapped[0], 255, 2);
  EXPECT_NEAR(mapped[1], 255, 2);
  const uint32_t bright = PackR10G10B10(kPQ1000, kPQ1000, kPQ1000);
  EXPECT_EQ(255, Convert(DXGI_FORMAT_R10G10B10A2_UNORM, CaptureEncoding::HDR10,
                         100.0f, &bright, 4)[2]);
  EXPECT_NEAR(188,
              Convert(DXGI_FORMAT_R10G10B10A2_UNORM, CaptureEncoding::HDR10,
                      200.0f, &white, 4)[0],
              2);

  // scRGB: 1.0 is 80 nits
  const auto twoHundred = Half(2.5f);
  EXPECT_EQ(255, Convert(DXGI_FORMAT_R16G16B16A16_FLOAT, CaptureEncoding::ScRGB,
                         200.0f, twoHundred.data(), 8)[0]);
  const auto half = Half(1.25f);
  EXPECT_NEAR(188,
              Convert(DXGI_FORMAT_R16G16B16A16_FLOAT, CaptureEncoding::ScRGB,
                      200.0f, half.data(), 8)[1],
              1);

  std::array<uint8_t, 4> unused{};
  EXPECT_FALSE(ConvertToRGBA8(bgra, 4, 1, 1, DXGI_FORMAT_R32_FLOAT,
                              CaptureEncoding::SDR, 80.0f, unused.data()));
}

TEST(CaptureTests, ConvertsToRec709LimitedRangeNV12) {
  struct Case {
    uint8_t R, G, B;
    uint8_t Y, U, V;
  };
  for (const auto &c :
       {Case{255, 255, 255, 235, 128, 128}, Case{0, 0, 0, 16, 128, 128},
        Case{255, 0, 0, 63, 102, 240}}) {
    std::array<uint8_t, 16> rgba{};
    for (size_t i = 0; i < 4; ++i) {
      rgba[i * 4] = c.R;
      rgba[i * 4 + 1] = c.G;
      rgba[i * 4 + 2] = c.B;
    }
    std::array<uint8_t, 6> nv12{};
    ConvertToNV12(rgba.data(), 8, 2, 2, nv12.data());
    EXPECT_EQ((std::array<uint8_t, 6>{c.Y, c.Y, c.Y, c.Y, c.U, c.V}), nv12);
  }
}

class CaptureIntegrationTests : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)));
    ASSERT_EQ(S_OK, MFStartup(MF_VERSION, MFSTARTUP_LITE));
    std::filesystem::remove_all(TempFolder());

    ComObject<IDXGIFactory4> factory;
    ASSERT_EQ(S_OK, CreateDXGIFactory1(IID_PPV_ARGS(factory.Assign())));
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(_device.Assign())))) {
      ComObject<IDXGIAdapter> warp;
      ASSERT_EQ(S_OK, factory->EnumWarpAdapter(IID_PPV_ARGS(warp.Assign())));
      ASSERT_EQ(S_OK, D3D12CreateDevice(warp, D3D_FEATURE_LEVEL_11_0,
                                        IID_PPV_ARGS(_device.Assign())));
    }
    const D3D12_COMMAND_QUEUE_DESC queueDesc{D3D12_COMMAND_LIST_TYPE_DIRECT};
    ASSERT_EQ(S_OK, _device->CreateCommandQueue(&queueDesc,
                                                IID_PPV_ARGS(_queue.Assign())));
    ASSERT_EQ(S_OK, _device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                         IID_PPV_ARGS(_fence.Assign())));
    ASSERT_EQ(S_OK, _device->CreateCommandAllocator(
                        D3D12_COMMAND_LIST_TYPE_DIRECT,
                        IID_PPV_ARGS(_allocator.Assign())));
    ASSERT_EQ(S_OK, _device->CreateCommandList(
                        0, D3D12_COMMAND_LIST_TYPE_DIRECT, _allocator, nullptr,
                        IID_PPV_ARGS(_list.Assign())));
    ASSERT_EQ(S_OK, _list->Close());
    // an odd size: the video crops to even dimensions
    const D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    const D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = 263,
        .Height = 257,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {1, 0},
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET};
    ASSERT_EQ(S_OK, _device->CreateCommittedResource(
                        &heap, D3D12_HEAP_FLAG_NONE, &desc,
                        D3D12_RESOURCE_STATE_PRESENT, nullptr,
                        IID_PPV_ARGS(_backBuffer.Assign())));
    const D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1};
    ASSERT_EQ(S_OK, _device->CreateDescriptorHeap(&rtvs,
                                                  IID_PPV_ARGS(_rtv.Assign())));
    _device->CreateRenderTargetView(_backBuffer, nullptr,
                                    _rtv->GetCPUDescriptorHandleForHeapStart());
    _captures = MakeCom<CaptureManager>(TempFolder(), false);
    _captures->RTSetDevice(_device, _queue, _fence);
  }

  void TearDown() override {
    if (_captures) _captures->RTShutDown();
    _captures.Clear();
    MFShutdown();
    CoUninitialize();
    std::error_code error;
    std::filesystem::remove_all(TempFolder(), error);
  }

  // draws one frame of a solid color, as a module's RTDraw would
  void Frame(std::array<float, 4> color, bool drawn = true,
             int64_t qpcTime = 0) {
    ASSERT_EQ(S_OK, _allocator->Reset());
    ASSERT_EQ(S_OK, _list->Reset(_allocator, nullptr));
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {.pResource = _backBuffer,
                       .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                       .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
                       .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET}};
    _list->ResourceBarrier(1, &barrier);
    _list->ClearRenderTargetView(_rtv->GetCPUDescriptorHandleForHeapStart(),
                                 color.data(), 0, nullptr);
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    _list->ResourceBarrier(1, &barrier);
    ASSERT_EQ(S_OK, _list->Close());
    ID3D12CommandList *lists[] = {_list};
    _queue->ExecuteCommandLists(1, lists);

    ++_ordinal;
    const MGDFFrameInfo frame{.FrameOrdinal = _ordinal,
                              .CompletedOrdinal = _fence->GetCompletedValue(),
                              .BackBuffer = _backBuffer,
                              .BackBufferDesc = _backBuffer->GetDesc()};
    _captures->RTCapture(frame, drawn, CaptureEncoding::SDR, 80.0f, qpcTime);
    ASSERT_EQ(S_OK, _queue->Signal(_fence, _ordinal));
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ASSERT_EQ(S_OK, _fence->SetEventOnCompletion(_ordinal, event));
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

  // draws black frames, past the video frame interval, until the recording
  // has written frames; the encoder starts some frames after the request
  bool RecordBlackFrames(IMGDFVideoRecording *recording, UINT64 frames) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (recording->GetFrameCount() < frames) {
      if (std::chrono::steady_clock::now() > deadline) return false;
      Frame({0.0f, 0.0f, 0.0f, 1.0f});
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return true;
  }

  ComObject<ID3D12Device10> _device;
  ComObject<ID3D12CommandQueue> _queue;
  ComObject<ID3D12Fence> _fence;
  ComObject<ID3D12CommandAllocator> _allocator;
  ComObject<ID3D12GraphicsCommandList> _list;
  ComObject<ID3D12Resource> _backBuffer;
  ComObject<ID3D12DescriptorHeap> _rtv;
  ComObject<CaptureManager> _captures;
  UINT64 _ordinal = 0;
};

TEST_F(CaptureIntegrationTests, ScreenshotSavesTheNextDrawnFrame) {
  ComObject<IMGDFCapture> capture;
  ASSERT_EQ(S_OK, _captures->TakeScreenshot("shots/first", capture.Assign()));
  EXPECT_EQ(E_PENDING, capture->GetResult());
  Frame({1.0f, 0.0f, 0.0f, 1.0f}, false);
  Frame({0.0f, 0.5f, 1.0f, 1.0f});
  Frame({0.0f, 0.0f, 0.0f, 1.0f}, false);
  ASSERT_TRUE(WaitForCompletion(capture.Get()));
  ASSERT_EQ(S_OK, capture->GetResult());

  const auto path = ComString<&IMGDFCapture::GetPath>(capture.Get()).str();
  EXPECT_EQ(TempFolder() / L"shots" / L"first.png", path);
  UINT width = 0, height = 0;
  const auto pixels = ReadPNG(path, width, height);
  EXPECT_EQ(263u, width);
  EXPECT_EQ(257u, height);
  EXPECT_EQ(0, pixels[0]);
  EXPECT_NEAR(128, pixels[1], 1);
  EXPECT_EQ(255, pixels[2]);
}

TEST_F(CaptureIntegrationTests, VideoRecordsDrawnFramesUntilStopped) {
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("clip", recording.Assign()));
  ASSERT_TRUE(RecordBlackFrames(recording.Get(), 3));
  recording->Stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  Frame({1.0f, 1.0f, 1.0f, 1.0f});
  ASSERT_TRUE(WaitForCompletion(recording.Get()));
  ASSERT_EQ(S_OK, recording->GetResult());
  EXPECT_EQ(0u, recording->GetDroppedFrameCount());

  UINT32 width = 0, height = 0;
  const auto luma = ReadVideoLuma(TempFolder() / L"clip.mp4", width, height);
  EXPECT_EQ(recording->GetFrameCount(), luma.size());
  for (const auto y : luma) EXPECT_NEAR(kBlackLuma, y, 4);
  EXPECT_EQ(262u, width);
  EXPECT_EQ(256u, height);
}

TEST_F(CaptureIntegrationTests, OneVideoRecordsAtATime) {
  ComObject<IMGDFVideoRecording> first;
  ASSERT_EQ(S_OK, _captures->StartVideo("first", first.Assign()));
  ComObject<IMGDFVideoRecording> second;
  EXPECT_EQ(E_NOT_VALID_STATE,
            _captures->StartVideo("second", second.Assign()));
  ComObject<IMGDFCapture> screenshot;
  ASSERT_EQ(S_OK, _captures->TakeScreenshot("during", screenshot.Assign()));
  ASSERT_TRUE(RecordBlackFrames(first.Get(), 1));
  ASSERT_TRUE(WaitForCompletion(screenshot.Get()));
  EXPECT_EQ(S_OK, screenshot->GetResult());

  first->Stop();
  EXPECT_EQ(S_OK, _captures->StartVideo("second", second.Assign()));
}

TEST_F(CaptureIntegrationTests, DeviceResetFailsCapturesUntilRecovered) {
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("reset", recording.Assign()));
  ASSERT_TRUE(RecordBlackFrames(recording.Get(), 2));
  ComObject<IMGDFCapture> screenshot;
  ASSERT_EQ(S_OK, _captures->TakeScreenshot("reset", screenshot.Assign()));

  _captures->RTReleaseDevice();
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED, recording->GetResult());
  ASSERT_TRUE(WaitForCompletion(recording.Get()));
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED, recording->GetResult());
  EXPECT_FALSE(std::filesystem::exists(TempFolder() / L"reset.mp4"));
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED, screenshot->GetResult());
  ComObject<IMGDFCapture> lostScreenshot;
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED,
            _captures->TakeScreenshot("lost", lostScreenshot.Assign()));
  ComObject<IMGDFVideoRecording> lostVideo;
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED,
            _captures->StartVideo("lost", lostVideo.Assign()));

  _captures->RTSetDevice(_device, _queue, _fence);
  ComObject<IMGDFVideoRecording> next;
  ASSERT_EQ(S_OK, _captures->StartVideo("next", next.Assign()));
  EXPECT_TRUE(RecordBlackFrames(next.Get(), 1));
}

TEST_F(CaptureIntegrationTests, ResetsApplyBeforeTheFirstDevice) {
  // created during a reset
  auto created = MakeCom<CaptureManager>(TempFolder() / L"created", true);
  ComObject<IMGDFCapture> rejected;
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED,
            created->TakeScreenshot("shot", rejected.Assign()));
  created->RTSetDevice(_device, _queue, _fence);
  ComObject<IMGDFCapture> accepted;
  EXPECT_EQ(S_OK, created->TakeScreenshot("shot", accepted.Assign()));
  created->RTShutDown();

  // reset before the first draw gave the recorder a device
  auto fresh = MakeCom<CaptureManager>(TempFolder() / L"fresh", false);
  ComObject<IMGDFCapture> pending;
  ASSERT_EQ(S_OK, fresh->TakeScreenshot("shot", pending.Assign()));
  fresh->RTReleaseDevice();
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED, pending->GetResult());
  EXPECT_EQ(DXGI_ERROR_DEVICE_REMOVED,
            fresh->TakeScreenshot("shot", rejected.Assign()));
  fresh->RTShutDown();
}

TEST_F(CaptureIntegrationTests, ReleasingAVideoStopsIt) {
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("released", recording.Assign()));
  ASSERT_TRUE(RecordBlackFrames(recording.Get(), 1));
  recording.Clear();
  for (int i = 0; i < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    Frame({1.0f, 1.0f, 1.0f, 1.0f});
  }
  // waits for the file
  _captures->RTShutDown();
  UINT32 width = 0, height = 0;
  const auto luma =
      ReadVideoLuma(TempFolder() / L"released.mp4", width, height);
  EXPECT_FALSE(luma.empty());
  for (const auto y : luma) EXPECT_NEAR(kBlackLuma, y, 4);
}

namespace {
class FrameTimer {
 public:
  FrameTimer() : _timer(CreateWaitableTimerW(nullptr, FALSE, nullptr)) {
    LARGE_INTEGER due;
    due.QuadPart = -1;
    if (_timer) SetWaitableTimer(_timer, &due, 20, nullptr, nullptr, FALSE);
  }
  ~FrameTimer() {
    if (_timer) {
      CancelWaitableTimer(_timer);
      CloseHandle(_timer);
    }
  }
  bool Wait() const {
    return WaitForSingleObject(_timer, 2000) == WAIT_OBJECT_0;
  }

 private:
  HANDLE _timer;
};
struct DecodedAudio {
  std::vector<float> Samples;
  int64_t First = 0, End = 0;
  UINT32 Rate = 0, Channels = 0;
};
DecodedAudio ReadRecordingAudio(const std::filesystem::path &path) {
  DecodedAudio result;
  ComObject<IMFSourceReader> reader;
  EXPECT_EQ(S_OK, MFCreateSourceReaderFromURL(path.c_str(), nullptr,
                                              reader.Assign()));
  if (!reader) return result;
  const DWORD stream = DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
  ComObject<IMFMediaType> type;
  EXPECT_EQ(S_OK, reader->GetNativeMediaType(stream, 0, type.Assign()));
  if (!type) return result;
  EXPECT_EQ(S_OK,
            type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &result.Rate));
  EXPECT_EQ(S_OK, type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &result.Channels));
  type.Clear();
  EXPECT_EQ(S_OK, MFCreateMediaType(type.Assign()));
  type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
  EXPECT_EQ(S_OK, reader->SetStreamSelection(
                      DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE));
  EXPECT_EQ(S_OK, reader->SetStreamSelection(stream, TRUE));
  EXPECT_EQ(S_OK, reader->SetCurrentMediaType(stream, nullptr, type));
  for (size_t block = 0; block < 10000; ++block) {
    DWORD flags = 0;
    int64_t time = 0;
    ComObject<IMFSample> sample;
    const auto hr =
        reader->ReadSample(stream, 0, nullptr, &flags, &time, sample.Assign());
    EXPECT_EQ(S_OK, hr);
    if (FAILED(hr)) break;
    if (sample) {
      if (result.Samples.empty()) result.First = time;
      int64_t duration = 0;
      sample->GetSampleDuration(&duration);
      result.End = time + duration;
      ComObject<IMFMediaBuffer> buffer;
      EXPECT_EQ(S_OK, sample->ConvertToContiguousBuffer(buffer.Assign()));
      BYTE *bytes = nullptr;
      DWORD count = 0;
      EXPECT_EQ(S_OK, buffer->Lock(&bytes, nullptr, &count));
      const auto first = reinterpret_cast<const float *>(bytes);
      result.Samples.insert(result.Samples.end(), first,
                            first + count / sizeof(float));
      buffer->Unlock();
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) return result;
  }
  ADD_FAILURE() << "Audio reader did not reach the end";
  return result;
}
struct VideoTimes {
  int64_t White = -1, End = 0;
};
VideoTimes ReadRecordingTimes(const std::filesystem::path &path) {
  VideoTimes result;
  ComObject<IMFSourceReader> reader;
  EXPECT_EQ(S_OK, MFCreateSourceReaderFromURL(path.c_str(), nullptr,
                                              reader.Assign()));
  if (!reader) return result;
  const DWORD stream = DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
  ComObject<IMFMediaType> type;
  MFCreateMediaType(type.Assign());
  type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
  EXPECT_EQ(S_OK, reader->SetStreamSelection(
                      DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE));
  EXPECT_EQ(S_OK, reader->SetStreamSelection(stream, TRUE));
  EXPECT_EQ(S_OK, reader->SetCurrentMediaType(stream, nullptr, type));
  for (size_t frame = 0; frame < 10000; ++frame) {
    DWORD flags = 0;
    int64_t time = 0;
    ComObject<IMFSample> sample;
    EXPECT_EQ(S_OK, reader->ReadSample(stream, 0, nullptr, &flags, &time,
                                       sample.Assign()));
    if (sample) {
      int64_t duration = 0;
      sample->GetSampleDuration(&duration);
      result.End = time + duration;
      ComObject<IMFMediaBuffer> buffer;
      sample->ConvertToContiguousBuffer(buffer.Assign());
      BYTE *bytes = nullptr;
      buffer->Lock(&bytes, nullptr, nullptr);
      if (bytes[0] > 200) result.White = time;
      buffer->Unlock();
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) return result;
  }
  ADD_FAILURE() << "Video reader did not reach the end";
  return result;
}
}  // namespace

class AudioCaptureIntegrationTests : public CaptureIntegrationTests {
 protected:
  void SetUp() override {
    CaptureIntegrationTests::SetUp();
    using namespace audio::openal_audio;
    _sound = MakeCom<OpenALSoundManagerComponentImpl>(
        AudioOptions{.Loopback = true, .Sources = 4});
    ASSERT_EQ(S_OK, _sound->Init());
    _captures->RTShutDown();
    _control = MakeCom<CaptureWorkerControl>();
    _captures = MakeCom<CaptureManager>(TempFolder(), false,
                                        _sound->CaptureSource(), _control);
    _captures->RTSetDevice(_device, _queue, _fence);
  }
  void TearDown() override {
    if (_control) SetEvent(_control->Resume);
    if (_sound) _sound->Stop();
    _sound.Clear();
    CaptureIntegrationTests::TearDown();
  }
  bool Prime(IMGDFVideoRecording *recording) {
    FrameTimer timer;
    const auto deadline = GetTickCount64() + 10000;
    while (recording->GetFrameCount() < 2 &&
           recording->GetResult() == E_PENDING) {
      if (GetTickCount64() >= deadline || !timer.Wait()) return false;
      Frame({0, 0, 0, 1});
    }
    return recording->GetFrameCount() >= 2;
  }
  void Finish(IMGDFVideoRecording *recording) {
    recording->Stop();
    Frame({0, 0, 0, 1});
    _captures->RTShutDown();
    ASSERT_EQ(S_OK, recording->GetResult());
  }
  ComObject<CaptureWorkerControl> _control;
  ComObject<audio::openal_audio::OpenALSoundManagerComponentImpl> _sound;
};

TEST_F(AudioCaptureIntegrationTests,
       VideoContainsStereoAudioInTimeWithAWhiteFrame) {
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("tone", recording.Assign()));
  Frame({0, 0, 0, 1}, false);
  ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(_control->Ready, 10000));
  const double start = 0.25;
  const MGDFSoundPcmDesc format{2, 48000, 4800};
  ComObject<IMGDFSoundPcmStream> pcm;
  ASSERT_EQ(S_OK, _sound->CreatePcmStream(&format, pcm.Assign()));
  std::vector<float> tone(9600);
  for (size_t i = 0; i < tone.size() / 2; ++i)
    tone[i * 2] = tone[i * 2 + 1] =
        0.25f * std::sin(float(i) * 6.28318530718f / 48.0f);
  ASSERT_EQ(4800u, pcm->Write(tone.data(), 4800));
  pcm->End();
  ComObject<IMGDFSoundCommandList> commands;
  ASSERT_EQ(S_OK, _sound->CreateCommandList(commands.Assign()));
  const MGDFVoiceDesc desc{.Bus = _sound->GetBus(MGDF_BUS_SOUND),
                           .Volume = 1,
                           .Pitch = 1,
                           .StartTime = start,
                           .GainHF = 1};
  commands->PlayPcm(pcm, &desc);
  _sound->Submit(commands);
  _sound->Update();
  LARGE_INTEGER base, frequency;
  QueryPerformanceCounter(&base);
  QueryPerformanceFrequency(&frequency);
  const auto baseTime =
      static_cast<int64_t>(double(base.QuadPart) * 1e7 / frequency.QuadPart);
  std::vector<float> mix(1600);
  for (int i = 0; i < 45; ++i) {
    const auto ticks = base.QuadPart + int64_t(i) * frequency.QuadPart / 60;
    _sound->RenderAt(mix.data(), 800, baseTime + int64_t(i) * 10000000 / 60);
    Frame(i == 15 ? std::array<float, 4>{1, 1, 1, 1}
                  : std::array<float, 4>{0, 0, 0, 1},
          true, ticks);
    Frame({0, 0, 0, 1}, false, ticks);
    const auto deadline = GetTickCount64() + 10000;
    while (recording->GetFrameCount() < UINT64(i)) {
      ASSERT_LT(GetTickCount64(), deadline);
      ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(_control->Progress, 10000));
    }
  }
  Finish(recording);
  const auto path = TempFolder() / L"tone.mp4";
  const auto decoded = ReadRecordingAudio(path);
  ASSERT_EQ(48000u, decoded.Rate);
  ASSERT_EQ(2u, decoded.Channels);
  const auto video = ReadRecordingTimes(path);
  ASSERT_GE(video.White, 0);
  const auto onset =
      std::find_if(decoded.Samples.begin(), decoded.Samples.end(),
                   [](float value) { return std::abs(value) > 0.05f; });
  ASSERT_NE(decoded.Samples.end(), onset);
  const auto frame = std::distance(decoded.Samples.begin(), onset) / 2;
  const double audioTime = double(decoded.First) / 1e7 + double(frame) / 48000;
  const double videoTime = double(video.White) / 1e7;
  std::cout << "Audio/video onset: audio=" << audioTime
            << " video=" << videoTime
            << " offset_ms=" << (audioTime - videoTime) * 1000
            << " first_audio=" << decoded.First << '\n';
  EXPECT_NEAR(audioTime, videoTime, 1.0 / 60);
}

TEST_F(AudioCaptureIntegrationTests,
       ADeviceGapBecomesSilenceWithoutShorteningTheTrack) {
  using namespace audio::openal_audio;
  auto source = MakeCom<TestOutputSource>();
  _sound->Stop();
  AudioOptions options{.Sources = 4};
  options.EndpointSource = source;
  _sound = MakeCom<OpenALSoundManagerComponentImpl>(options);
  ASSERT_EQ(S_OK, _sound->Init());
  _captures->RTShutDown();
  _control = MakeCom<CaptureWorkerControl>();
  _captures = MakeCom<CaptureManager>(TempFolder(), false,
                                      _sound->CaptureSource(), _control);
  _captures->RTSetDevice(_device, _queue, _fence);
  const MGDFSoundPcmDesc format{2, 48000, 96000};
  ComObject<IMGDFSoundPcmStream> pcm;
  ASSERT_EQ(S_OK, _sound->CreatePcmStream(&format, pcm.Assign()));
  std::vector<float> tone(192000);
  for (size_t i = 0; i < tone.size() / 2; ++i)
    tone[i * 2] = tone[i * 2 + 1] =
        0.25f * std::sin(float(i) * 6.28318530718f / 48);
  ASSERT_EQ(96000u, pcm->Write(tone.data(), 96000));
  ComObject<IMGDFSoundCommandList> commands;
  ASSERT_EQ(S_OK, _sound->CreateCommandList(commands.Assign()));
  const MGDFVoiceDesc desc{.Bus = 1, .Volume = 1, .Pitch = 1, .GainHF = 1};
  commands->PlayPcm(pcm, &desc);
  _sound->Submit(commands);
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("gap", recording.Assign()));
  ASSERT_TRUE(Prime(recording));
  struct ResumeOnExit {
    HANDLE Resume;
    ~ResumeOnExit() { SetEvent(Resume); }
  } resume{source->State->RenderResume};
  ResetEvent(source->State->RenderResume);
  ResetEvent(source->State->Rendering);
  ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(source->State->Rendering, 2000));
  FrameTimer timer;
  for (int i = 0; i < 12; ++i) {
    ASSERT_TRUE(timer.Wait());
    Frame({0, 0, 0, 1});
  }
  SetEvent(source->State->RenderResume);
  for (int i = 0; i < 15; ++i) {
    ASSERT_TRUE(timer.Wait());
    Frame({0, 0, 0, 1});
  }
  Finish(recording);
  const auto path = TempFolder() / L"gap.mp4";
  const auto decoded = ReadRecordingAudio(path);
  const auto video = ReadRecordingTimes(path);
  ASSERT_EQ(48000u, decoded.Rate);
  EXPECT_NEAR(double(decoded.End), double(video.End),
              10000000.0 * 1024 / 48000);
  size_t quiet = 0, longest = 0;
  bool heardBefore = false, heardAfter = false;
  for (size_t i = 0; i < decoded.Samples.size(); i += 2) {
    if (std::abs(decoded.Samples[i]) < 0.002f) {
      if (heardBefore) {
        ++quiet;
        longest = (std::max)(longest, quiet);
      }
    } else {
      if (longest > 4800) heardAfter = true;
      heardBefore = true;
      quiet = 0;
    }
  }
  EXPECT_GT(longest, 7200u);
  EXPECT_LT(longest, 16000u);
  EXPECT_TRUE(heardAfter);
  std::cout << "gap silence_ms=" << double(longest) * 1000 / 48000
            << " track_difference_ms="
            << double(decoded.End - video.End) / 10000 << '\n';
}
TEST_F(AudioCaptureIntegrationTests,
       ASlowRecorderDropsAudioWithoutStoppingTheOutput) {
  using namespace audio::openal_audio;
  _sound->Stop();
  _sound = MakeCom<OpenALSoundManagerComponentImpl>(
      AudioOptions{.Sources = 4, .OutputFile = TempFolder() / L"slow.wav"});
  ASSERT_EQ(S_OK, _sound->Init());
  _captures->RTShutDown();
  _control = MakeCom<CaptureWorkerControl>();
  ResetEvent(_control->Resume);
  _captures = MakeCom<CaptureManager>(TempFolder(), false,
                                      _sound->CaptureSource(), _control);
  _captures->RTSetDevice(_device, _queue, _fence);
  ComObject<IMGDFVideoRecording> recording;
  ASSERT_EQ(S_OK, _captures->StartVideo("slow", recording.Assign()));
  Frame({0, 0, 0, 1}, false);
  ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(_control->Ready, 10000));
  const double start = _sound->GetAudioTime();
  FrameTimer timer;
  const auto deadline = GetTickCount64() + 5000;
  while (_sound->GetAudioTime() < start + 0.85) {
    ASSERT_LT(GetTickCount64(), deadline);
    ASSERT_TRUE(timer.Wait());
    Frame({0, 0, 0, 1});
  }
  MGDFSoundStats stats{};
  _sound->GetStats(&stats);
  EXPECT_GT(stats.DroppedAudioBlocks, 0u);
  EXPECT_EQ(0u, recording->GetFrameCount());
  SetEvent(_control->Resume);
  Finish(recording);
  EXPECT_GT(recording->GetFrameCount(), 0u);
  const auto decoded = ReadRecordingAudio(TempFolder() / L"slow.mp4");
  EXPECT_EQ(48000u, decoded.Rate);
  EXPECT_FALSE(decoded.Samples.empty());
}
