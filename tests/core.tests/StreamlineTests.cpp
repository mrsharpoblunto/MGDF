#include "stdafx.h"

#include <DirectXPackedVector.h>
#include <dxgi1_6.h>

#include <MGDF/D3D12CommandList.hpp>
#include <filesystem>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFStreamline.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {

constexpr UINT32 AllFeatures = MGDF_GRAPHICS_FEATURE_UPSCALE |
                               MGDF_GRAPHICS_FEATURE_FRAME_GENERATION |
                               MGDF_GRAPHICS_FEATURE_LOW_LATENCY;

struct TestDevice {
  ComObject<IDXGIAdapter1> Adapter;
  ComObject<ID3D12Device> Device;
};

std::optional<TestDevice> CreateDevice(bool nvidiaOnly) {
  ComObject<IDXGIFactory6> factory;
  if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.Assign()))))
    return std::nullopt;
  for (UINT i = 0;; ++i) {
    TestDevice device;
    if (factory->EnumAdapterByGpuPreference(
            i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(device.Adapter.Assign())) == DXGI_ERROR_NOT_FOUND)
      return std::nullopt;
    DXGI_ADAPTER_DESC1 desc{};
    device.Adapter->GetDesc1(&desc);
    if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) ||
        (nvidiaOnly && desc.VendorId != 0x10DE))
      continue;
    if (SUCCEEDED(D3D12CreateDevice(device.Adapter, D3D_FEATURE_LEVEL_11_0,
                                    IID_PPV_ARGS(device.Device.Assign()))))
      return device;
  }
}

ComObject<ID3D12Resource> CreateTexture(ID3D12Device *device, UINT width,
                                        UINT height, DXGI_FORMAT format,
                                        D3D12_RESOURCE_FLAGS flags,
                                        D3D12_RESOURCE_STATES state) {
  const D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
  const D3D12_RESOURCE_DESC desc{
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
      .Width = width,
      .Height = height,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = format,
      .SampleDesc = {1, 0},
      .Flags = flags};
  ComObject<ID3D12Resource> texture;
  if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                             state, nullptr,
                                             IID_PPV_ARGS(texture.Assign()))))
    return {};
  return texture;
}

D3D12_RESOURCE_BARRIER Transition(ID3D12Resource *resource,
                                  D3D12_RESOURCE_STATES before,
                                  D3D12_RESOURCE_STATES after) {
  return {.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Transition = {.pResource = resource,
                         .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                         .StateBefore = before,
                         .StateAfter = after}};
}

MGDFFrameConstants StaticCamera(bool reset) {
  MGDFFrameConstants constants{};
  for (const auto matrix :
       {constants.ViewToClip, constants.ClipToView,
        constants.ClipToPreviousClip, constants.PreviousClipToClip}) {
    for (int i = 0; i < 4; ++i) matrix[i * 5] = 1.0f;
  }
  constants.MotionVectorScale[0] = 1.0f / 960.0f;
  constants.MotionVectorScale[1] = 1.0f / 540.0f;
  constants.CameraUp[1] = 1.0f;
  constants.CameraRight[0] = 1.0f;
  constants.CameraForward[2] = 1.0f;
  constants.Near = 0.1f;
  constants.Far = 1000.0f;
  constants.VerticalFOV = 1.0f;
  constants.AspectRatio = 16.0f / 9.0f;
  constants.CameraMotionIncluded = TRUE;
  constants.ResetHistory = reset ? TRUE : FALSE;
  return constants;
}

}  // namespace

TEST(StreamlineTests, InactiveStreamlineReportsNoSupportAndIgnoresFrameCalls) {
  Streamline streamline(AllFeatures, L"");
  const auto support = streamline.GetSupport();
  EXPECT_EQ(0u, support.Features);
  EXPECT_EQ(0u, support.MaxGeneratedFrames);
  MGDFUpscaleRenderSize size{};
  EXPECT_EQ(E_NOTIMPL, streamline.GetUpscaleRenderSize(MGDF_UPSCALE_MODE_DLAA,
                                                       1920, 1080, &size));
  streamline.RTBeginFrame(MGDF_LOW_LATENCY_BOOST);
  streamline.RTSetFrameMarker(MGDF_FRAME_MARKER_SIMULATION_START);
  const auto constants = StaticCamera(true);
  streamline.RTSetFrameConstants(&constants);
  const MGDFUpscaleDesc upscale{};
  EXPECT_EQ(E_NOTIMPL, streamline.RTUpscale(nullptr, &upscale));
  const MGDFFrameGenerationDesc generation{};
  EXPECT_EQ(S_FALSE, streamline.RTSetFrameGenerationInputs(&generation));
  EXPECT_FALSE(streamline.RTPrepareSwapChain(2, DXGI_FORMAT_R10G10B10A2_UNORM));
  EXPECT_EQ(1u, streamline.RTGetSyncInterval(1));
  streamline.RTBeforePresent();
  streamline.RTAfterPresent();
  EXPECT_EQ(0u, streamline.GetFrameStats().PresentedFrames);
  EXPECT_EQ(0u, streamline.GetLatencyPingMessage());
}

TEST(StreamlineIntegrationTests, MissingPluginsLeaveStreamlineInactive) {
  const auto device = CreateDevice(false);
  if (!device) GTEST_SKIP() << "No hardware D3D12 adapter";
  const auto empty =
      std::filesystem::temp_directory_path() /
      ("mgdf-streamline-" + std::to_string(GetCurrentProcessId()));
  std::filesystem::create_directories(empty);
  {
    Streamline streamline(AllFeatures, empty.wstring() + L"\\");
    streamline.RTInit(device->Device, device->Adapter);
    EXPECT_FALSE(streamline.RTIsActive());
    EXPECT_FALSE(streamline.RTGetProxyDevice());
    EXPECT_EQ(0u, streamline.GetSupport().Features);
    MGDFUpscaleRenderSize size{};
    EXPECT_EQ(E_NOTIMPL, streamline.GetUpscaleRenderSize(
                             MGDF_UPSCALE_MODE_QUALITY, 1920, 1080, &size));
    streamline.RTShutdown();
  }
  std::error_code error;
  std::filesystem::remove_all(empty, error);
}

TEST(StreamlineIntegrationTests, UpscaleReproducesAConstantImage) {
  Resources::Instance(GetModuleHandleW(nullptr));
  const auto directory = Resources::Instance().RootDir();
  if (!std::filesystem::exists(directory + L"sl.interposer.dll"))
    GTEST_SKIP() << "Streamline runtime is not in the test folder";
  const auto device = CreateDevice(true);
  if (!device) GTEST_SKIP() << "No NVIDIA D3D12 adapter";

  Streamline streamline(MGDF_GRAPHICS_FEATURE_UPSCALE, directory);
  streamline.RTInit(device->Device, device->Adapter);
  ASSERT_TRUE(streamline.GetSupport().Features & MGDF_GRAPHICS_FEATURE_UPSCALE);

  MGDFUpscaleRenderSize size{};
  ASSERT_EQ(S_OK, streamline.GetUpscaleRenderSize(MGDF_UPSCALE_MODE_DLAA, 1920,
                                                  1080, &size));
  EXPECT_EQ(1920u, size.Width);
  EXPECT_EQ(1080u, size.Height);
  ASSERT_EQ(S_OK, streamline.GetUpscaleRenderSize(MGDF_UPSCALE_MODE_PERFORMANCE,
                                                  1920, 1080, &size));
  EXPECT_EQ(960u, size.Width);
  EXPECT_EQ(540u, size.Height);
  EXPECT_LE(size.MinWidth, size.Width);
  EXPECT_GE(size.MaxWidth, size.Width);

  {
    ID3D12Device *d3d = device->Device;
    const auto proxy = streamline.RTGetProxyDevice();
    ASSERT_TRUE(proxy);
    const D3D12_COMMAND_QUEUE_DESC queueDesc{D3D12_COMMAND_LIST_TYPE_DIRECT};
    ComObject<ID3D12CommandQueue> proxyQueue;
    ASSERT_EQ(S_OK, proxy->CreateCommandQueue(
                        &queueDesc, IID_PPV_ARGS(proxyQueue.Assign())));
    const auto queue = streamline.RTGetNative<ID3D12CommandQueue>(proxyQueue);
    ASSERT_TRUE(queue);
    ComObject<ID3D12CommandAllocator> allocator;
    ASSERT_EQ(S_OK,
              d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          IID_PPV_ARGS(allocator.Assign())));
    ComObject<ID3D12Fence> fence;
    ASSERT_EQ(S_OK, d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(fence.Assign())));
    UINT64 fenceValue = 0;
    const auto execute = [&](D3D12CommandList &list) {
      ASSERT_EQ(S_OK, list.Close());
      ID3D12CommandList *lists[] = {list.Get()};
      queue->ExecuteCommandLists(1, lists);
      ASSERT_EQ(S_OK, queue->Signal(fence, ++fenceValue));
      ASSERT_EQ(S_OK, fence->SetEventOnCompletion(fenceValue, nullptr));
      ASSERT_EQ(S_OK, allocator->Reset());
    };

    constexpr auto input = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    const auto color =
        CreateTexture(d3d, 960, 540, DXGI_FORMAT_R16G16B16A16_FLOAT,
                      D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                      D3D12_RESOURCE_STATE_RENDER_TARGET);
    const auto depth = CreateTexture(d3d, 960, 540, DXGI_FORMAT_R32_FLOAT,
                                     D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                     D3D12_RESOURCE_STATE_RENDER_TARGET);
    const auto motion = CreateTexture(d3d, 960, 540, DXGI_FORMAT_R16G16_FLOAT,
                                      D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);
    const auto output =
        CreateTexture(d3d, 1920, 1080, DXGI_FORMAT_R16G16B16A16_FLOAT,
                      D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ASSERT_TRUE(color && depth && motion && output);

    const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                              3};
    ComObject<ID3D12DescriptorHeap> heap;
    ASSERT_EQ(S_OK, d3d->CreateDescriptorHeap(&heapDesc,
                                              IID_PPV_ARGS(heap.Assign())));
    D3D12CommandList list;
    ASSERT_EQ(S_OK, list.Create(d3d, allocator));
    const float sceneColor[4] = {0.25f, 0.5f, 0.75f, 1.0f};
    const float zero[4] = {};
    auto handle = heap->GetCPUDescriptorHandleForHeapStart();
    const auto rtvSize =
        d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (const auto &[texture, clear] :
         {std::pair{color.Get(), sceneColor}, std::pair{depth.Get(), zero},
          std::pair{motion.Get(), zero}}) {
      d3d->CreateRenderTargetView(texture, nullptr, handle);
      list.Get()->ClearRenderTargetView(handle, clear, 0, nullptr);
      const auto barrier =
          Transition(texture, D3D12_RESOURCE_STATE_RENDER_TARGET, input);
      list.Get()->ResourceBarrier(1, &barrier);
      handle.ptr += rtvSize;
    }
    execute(list);

    const MGDFUpscaleDesc desc{
        .Mode = MGDF_UPSCALE_MODE_PERFORMANCE,
        .RenderWidth = 960,
        .RenderHeight = 540,
        .OutputWidth = 1920,
        .OutputHeight = 1080,
        .ColorHDR = TRUE,
        .ColorIn = {color, input},
        .ColorOut = {output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
        .Depth = {depth, input},
        .MotionVectors = {motion, input}};

    ComObject<ID3D12GraphicsCommandList> untracked;
    ASSERT_EQ(S_OK, d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           allocator, nullptr,
                                           IID_PPV_ARGS(untracked.Assign())));
    streamline.RTBeginFrame(MGDF_LOW_LATENCY_OFF);
    EXPECT_EQ(E_INVALIDARG, streamline.RTUpscale(untracked, &desc));
    ASSERT_EQ(S_OK, untracked->Close());
    ASSERT_EQ(S_OK, list.Reset(allocator));
    EXPECT_EQ(E_ILLEGAL_METHOD_CALL, streamline.RTUpscale(list.Get(), &desc));

    for (int frame = 0; frame < 8; ++frame) {
      if (frame) {
        streamline.RTBeginFrame(MGDF_LOW_LATENCY_OFF);
        ASSERT_EQ(S_OK, list.Reset(allocator));
      }
      const auto constants = StaticCamera(frame == 0);
      streamline.RTSetFrameConstants(&constants);
      ASSERT_EQ(S_OK, streamline.RTUpscale(list.Get(), &desc));
      execute(list);
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 bytes = 0;
    const auto outputDesc = output->GetDesc();
    d3d->GetCopyableFootprints(&outputDesc, 0, 1, 0, &footprint, nullptr,
                               nullptr, &bytes);
    const D3D12_HEAP_PROPERTIES readbackHeap{D3D12_HEAP_TYPE_READBACK};
    const D3D12_RESOURCE_DESC bufferDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = bytes,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
    ComObject<ID3D12Resource> readback;
    ASSERT_EQ(S_OK, d3d->CreateCommittedResource(
                        &readbackHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                        IID_PPV_ARGS(readback.Assign())));
    ASSERT_EQ(S_OK, list.Reset(allocator));
    auto barrier = Transition(output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                              D3D12_RESOURCE_STATE_COPY_SOURCE);
    list.Get()->ResourceBarrier(1, &barrier);
    const D3D12_TEXTURE_COPY_LOCATION source{
        .pResource = output,
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0};
    const D3D12_TEXTURE_COPY_LOCATION target{
        .pResource = readback,
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = footprint};
    list.Get()->CopyTextureRegion(&target, 0, 0, 0, &source, nullptr);
    execute(list);

    void *mapped = nullptr;
    ASSERT_EQ(S_OK, readback->Map(0, nullptr, &mapped));
    const auto *bytesOut = static_cast<const uint8_t *>(mapped);
    for (const UINT y : {10u, 540u, 1070u}) {
      for (const UINT x : {10u, 960u, 1910u}) {
        SCOPED_TRACE(testing::Message() << x << "," << y);
        const auto *pixel =
            reinterpret_cast<const DirectX::PackedVector::HALF *>(
                bytesOut + footprint.Footprint.RowPitch * y + x * 8);
        for (int c = 0; c < 3; ++c) {
          EXPECT_NEAR(sceneColor[c],
                      DirectX::PackedVector::XMConvertHalfToFloat(pixel[c]),
                      0.02f);
        }
      }
    }
    readback->Unmap(0, nullptr);
  }
  streamline.RTShutdown();
}
