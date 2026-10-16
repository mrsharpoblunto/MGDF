#include "stdafx.h"

#include <dxgi1_4.h>

#include <MGDF/D3D12CommandList.hpp>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFTimer.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {
class TimerMetric final : public ComBase<IMGDFMetric> {
 public:
  void Record(double value, const MGDFTags *tags) override {
    Samples.emplace(tags->Values[0], value);
  }
  std::map<std::string, double> Samples;
};
}  // namespace

TEST(D3D12TimerIntegrationTests,
     CompletedScopesKeepTimesAcrossUnfinishedSample) {
  Resources::Instance(GetModuleHandleW(nullptr));
  ComObject<IDXGIFactory4> factory;
  ASSERT_EQ(S_OK, CreateDXGIFactory1(IID_PPV_ARGS(factory.Assign())));
  ComObject<ID3D12Device10> device;
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                               IID_PPV_ARGS(device.Assign())))) {
    ComObject<IDXGIAdapter> warp;
    ASSERT_EQ(S_OK, factory->EnumWarpAdapter(IID_PPV_ARGS(warp.Assign())));
    ASSERT_EQ(S_OK, D3D12CreateDevice(warp, D3D_FEATURE_LEVEL_11_0,
                                      IID_PPV_ARGS(device.Assign())));
  }
  const D3D12_COMMAND_QUEUE_DESC queueDesc{D3D12_COMMAND_LIST_TYPE_DIRECT};
  ComObject<ID3D12CommandQueue> queue;
  ASSERT_EQ(S_OK, device->CreateCommandQueue(&queueDesc,
                                             IID_PPV_ARGS(queue.Assign())));
  ComObject<ID3D12CommandAllocator> allocator;
  ASSERT_EQ(S_OK,
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(allocator.Assign())));
  D3D12CommandList list;
  ASSERT_EQ(S_OK, list.Create(device, allocator));
  ComObject<Timer> timer;
  ASSERT_EQ(S_OK, Timer::TryCreate(5, timer));
  ASSERT_EQ(S_OK, timer->InitFromDevice12(device, queue, 1));
  const auto metric = MakeCom<TimerMetric>();
  ComObject<IMGDFPerformanceCounter> counter;
  ASSERT_EQ(S_OK,
            timer->CreateGPUCounter(metric, list.Get(), counter.Assign()));

  const D3D12_HEAP_PROPERTIES properties{D3D12_HEAP_TYPE_DEFAULT};
  const D3D12_RESOURCE_DESC buffer{.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                                   .Width = 4096,
                                   .Height = 1,
                                   .DepthOrArraySize = 1,
                                   .MipLevels = 1,
                                   .SampleDesc = {1, 0},
                                   .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
  ComObject<ID3D12Resource> source, target;
  ASSERT_EQ(S_OK, device->CreateCommittedResource(
                      &properties, D3D12_HEAP_FLAG_NONE, &buffer,
                      D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
                      IID_PPV_ARGS(source.Assign())));
  ASSERT_EQ(S_OK, device->CreateCommittedResource(
                      &properties, D3D12_HEAP_FLAG_NONE, &buffer,
                      D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                      IID_PPV_ARGS(target.Assign())));
  timer->BeginD3D12Frame({.FrameOrdinal = 1, .FramesInFlight = 1});
  const auto begin = [&](const char *name,
                         ComObject<IMGDFPerformanceCounterScope> &scope) {
    const char *key = "scope";
    const MGDFTags tags{&key, &name, 1};
    return counter->Begin(&tags, scope.Assign());
  };
  ComObject<IMGDFPerformanceCounterScope> outer, first, unfinished, last;
  ASSERT_EQ(S_OK, begin("outer", outer));
  ASSERT_EQ(S_OK, begin("first", first));
  list.Get()->CopyBufferRegion(target, 0, source, 0, buffer.Width);
  first.Clear();
  ASSERT_EQ(S_OK, begin("unfinished", unfinished));
  ASSERT_EQ(S_OK, begin("last", last));
  list.Get()->CopyBufferRegion(target, 0, source, 0, buffer.Width);
  last.Clear();
  outer.Clear();
  ASSERT_EQ(S_OK, list.Close());
  ID3D12CommandList *lists[]{list.Get()};
  queue->ExecuteCommandLists(1, lists);
  ASSERT_EQ(S_OK, timer->EndD3D12Frame());
  unfinished.Clear();
  EXPECT_TRUE(metric->Samples.empty());

  ComObject<ID3D12Fence> fence;
  ASSERT_EQ(S_OK, device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                      IID_PPV_ARGS(fence.Assign())));
  ASSERT_EQ(S_OK, queue->Signal(fence, 1));
  const HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  ASSERT_NE(nullptr, event);
  const auto completion = fence->SetEventOnCompletion(1, event);
  const auto wait =
      SUCCEEDED(completion) ? WaitForSingleObject(event, 30000) : WAIT_FAILED;
  CloseHandle(event);
  ASSERT_EQ(WAIT_OBJECT_0, wait);
  timer->BeginD3D12Frame(
      {.FrameOrdinal = 2, .CompletedOrdinal = 1, .FramesInFlight = 1});
  ASSERT_EQ(3u, metric->Samples.size());
  EXPECT_EQ(0u, metric->Samples.count("unfinished"));
  EXPECT_GT(metric->Samples.at("first"), 0.0);
  EXPECT_GT(metric->Samples.at("last"), 0.0);
  EXPECT_GE(metric->Samples.at("outer"),
            metric->Samples.at("first") + metric->Samples.at("last"));
}
