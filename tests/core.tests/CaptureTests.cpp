#include "stdafx.h"

#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wincodec.h>

#include <MGDF/ComObject.hpp>
#include <array>
#include <chrono>
#include <filesystem>
#include <thread>

#include "../../src/core/core.impl/MGDFCaptures.hpp"

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
  void Frame(std::array<float, 4> color, bool drawn = true) {
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
    _captures->RTCapture(frame, drawn, CaptureEncoding::SDR, 80.0f);
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
