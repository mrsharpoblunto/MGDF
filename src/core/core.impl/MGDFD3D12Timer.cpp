#include "StdAfx.h"

#include "MGDFD3D12Timer.hpp"

#include <MGDF/D3D12CommandList.hpp>

#include "../common/MGDFLoggerImpl.hpp"

namespace MGDF {
namespace core {
namespace {

bool Recording(ID3D12GraphicsCommandList *list, UINT64 &generation) {
  if (!list) return false;
  D3D12CommandListRecording state{};
  UINT size = sizeof(state);
  if (FAILED(
          list->GetPrivateData(D3D12CommandListRecordingID, &size, &state)) ||
      size != sizeof(state) || !state.Open)
    return false;
  generation = state.Generation;
  return true;
}

class D3D12CounterScope : public ComBase<IMGDFPerformanceCounterScope> {
 public:
  D3D12CounterScope(D3D12CounterManager &manager,
                    std::shared_ptr<D3D12CounterManager::Sample> sample)
      : _manager(manager), _sample(std::move(sample)) {}
  ~D3D12CounterScope() { _manager.End(_sample); }

 private:
  D3D12CounterManager &_manager;
  std::shared_ptr<D3D12CounterManager::Sample> _sample;
};

}  // namespace

D3D12PerformanceCounter::D3D12PerformanceCounter(
    IMGDFMetric *metric, Timer &timer, D3D12CounterManager &manager,
    ID3D12GraphicsCommandList *list)
    : CounterBase(metric, timer), _manager(manager), _list(list, true) {}
D3D12PerformanceCounter::~D3D12PerformanceCounter() { _manager.Remove(this); }
HRESULT D3D12PerformanceCounter::DoBegin(
    std::map<std::string, std::string> &tags,
    IMGDFPerformanceCounterScope **scope) {
  return _manager.Begin(this, tags, scope);
}
void D3D12PerformanceCounter::Record(
    double value, const std::map<std::string, std::string> &tags) {
  AddSample(value);
  std::vector<const char *> names, values;
  for (auto &tag : tags) {
    names.push_back(tag.first.c_str());
    values.push_back(tag.second.c_str());
  }
  const MGDFTags metricTags{names.data(), values.data(), names.size()};
  _metric->Record(value, &metricTags);
}

HRESULT D3D12CounterManager::Init(ID3D12Device10 *device,
                                  ID3D12CommandQueue *queue, UINT slots) {
  Reset();
  std::lock_guard frameLock(_frameMutex);
  _device = MakeComFromPtr<ID3D12Device10>(device);
  _queue = MakeComFromPtr<ID3D12CommandQueue>(queue);
  HRESULT result = queue->GetTimestampFrequency(&_frequency);
  if (FAILED(result) || !_frequency) return E_FAIL;
  for (UINT i = 0; i < slots; ++i) {
    auto &slot = *_slots.emplace_back(std::make_unique<Slot>());
    const D3D12_QUERY_HEAP_DESC heap{D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
                                     MaxPairs * 2, 0};
    result = device->CreateQueryHeap(&heap, IID_PPV_ARGS(slot.Heap.Assign()));
    if (FAILED(result)) return result;
    const D3D12_HEAP_PROPERTIES properties{.Type = D3D12_HEAP_TYPE_READBACK};
    const D3D12_RESOURCE_DESC desc{.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                                   .Width = MaxPairs * 2 * sizeof(UINT64),
                                   .Height = 1,
                                   .DepthOrArraySize = 1,
                                   .MipLevels = 1,
                                   .SampleDesc = {1, 0},
                                   .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
    result = device->CreateCommittedResource(
        &properties, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(slot.Readback.Assign()));
    if (FAILED(result)) return result;
    void *mapped = nullptr;
    const D3D12_RANGE readRange{0, static_cast<SIZE_T>(desc.Width)};
    result = slot.Readback->Map(0, &readRange, &mapped);
    if (FAILED(result)) return result;
    slot.Results = static_cast<const UINT64 *>(mapped);
    result = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(slot.Allocator.Assign()));
    if (FAILED(result)) return result;
    result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       slot.Allocator, nullptr,
                                       IID_PPV_ARGS(slot.List.Assign()));
    if (FAILED(result)) return result;
    result = slot.List->Close();
    if (FAILED(result)) return result;
  }
  return S_OK;
}

void D3D12CounterManager::Reset() {
  std::lock_guard frameLock(_frameMutex);
  _recording = false;
  _frame = {};
  for (auto &slot : _slots) {
    if (slot->Results) {
      const D3D12_RANGE written{0, 0};
      slot->Readback->Unmap(0, &written);
    }
  }
  // Pending samples own counters, whose destructors take _mutex.
  _slots.clear();
  std::lock_guard lock(_mutex);
  for (auto counter : _counters) counter->Reset();
  _queue.Clear();
  _device.Clear();
}

HRESULT D3D12CounterManager::Create(IMGDFMetric *metric,
                                    ID3D12GraphicsCommandList *list,
                                    IMGDFPerformanceCounter **counter) {
  std::lock_guard frameLock(_frameMutex);
  if (!counter) return E_POINTER;
  *counter = nullptr;
  if (!metric || !list || list->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT)
    return E_INVALIDARG;
  D3D12CommandListRecording state{};
  UINT size = sizeof(state);
  if (FAILED(
          list->GetPrivateData(D3D12CommandListRecordingID, &size, &state)) ||
      size != sizeof(state)) {
    LOG("D3D12 GPU counters require MGDF::D3D12CommandList recording tracking",
        MGDF_LOG_ERROR);
    return E_INVALIDARG;
  }
  ComObject<ID3D12Device10> device;
  if (!_device || FAILED(list->GetDevice(IID_PPV_ARGS(device.Assign()))) ||
      device.Get() != _device.Get())
    return E_INVALIDARG;
  auto created = MakeCom<D3D12PerformanceCounter>(metric, _timer, *this, list);
  std::lock_guard lock(_mutex);
  _counters.insert(created);
  created.AddRawRef(counter);
  return S_OK;
}

HRESULT D3D12CounterManager::Begin(D3D12PerformanceCounter *counter,
                                   std::map<std::string, std::string> &tags,
                                   IMGDFPerformanceCounterScope **scope) {
  std::lock_guard frameLock(_frameMutex);
  if (!scope) return E_POINTER;
  *scope = nullptr;
  UINT64 generation = 0;
  if (!_recording || !Recording(counter->List(), generation))
    return E_INVALIDARG;
  auto &slot = *_slots[_frame.FrameSlot];
  if (slot.Pending.size() == MaxPairs) return E_OUTOFMEMORY;
  auto sample = std::make_shared<Sample>();
  sample->Counter = MakeComFromPtr<D3D12PerformanceCounter>(counter);
  sample->Tags = std::move(tags);
  sample->Frame = _frame.FrameOrdinal;
  sample->Generation = generation;
  sample->Index = static_cast<UINT>(slot.Pending.size()) * 2;
  slot.Pending.push_back(sample);
  counter->List()->EndQuery(slot.Heap, D3D12_QUERY_TYPE_TIMESTAMP,
                            sample->Index);
  auto created = MakeCom<D3D12CounterScope>(*this, sample);
  created.AddRawRef(scope);
  return S_OK;
}

void D3D12CounterManager::End(const std::shared_ptr<Sample> &sample) {
  std::lock_guard frameLock(_frameMutex);
  UINT64 generation = 0;
  if (!_recording || sample->Frame != _frame.FrameOrdinal ||
      !Recording(sample->Counter->List(), generation) ||
      generation != sample->Generation) {
    LOG("D3D12 GPU scope ended after its command list closed/reset or its "
        "frame ended; sample discarded",
        MGDF_LOG_ERROR);
    return;
  }
  auto &slot = *_slots[_frame.FrameSlot];
  sample->Counter->List()->EndQuery(slot.Heap, D3D12_QUERY_TYPE_TIMESTAMP,
                                    sample->Index + 1);
  sample->Ended = true;
}

void D3D12CounterManager::BeginFrame(const MGDFFrameInfo &frame) {
  std::lock_guard frameLock(_frameMutex);
  _frame = frame;
  auto &slot = *_slots[frame.FrameSlot];
  _ASSERTE(frame.CompletedOrdinal >= slot.Fence);
  for (const auto &sample : slot.Pending) {
    if (!sample->Ended) continue;
    const auto begin = slot.Results[sample->Index];
    const auto end = slot.Results[sample->Index + 1];
    if (end >= begin)
      sample->Counter->Record(static_cast<double>(end - begin) / _frequency,
                              sample->Tags);
  }
  slot.Pending.clear();
  _recording = true;
}

HRESULT D3D12CounterManager::EndFrame() {
  std::lock_guard frameLock(_frameMutex);
  _recording = false;
  auto &slot = *_slots[_frame.FrameSlot];
  slot.Fence = _frame.FrameOrdinal;
  if (slot.Pending.empty()) return S_OK;
  auto result = slot.Allocator->Reset();
  if (FAILED(result)) return result;
  result = slot.List->Reset(slot.Allocator, nullptr);
  if (FAILED(result)) return result;
  for (const auto &sample : slot.Pending) {
    // Never resolve an uninitialized end query from a rejected or leaked scope.
    if (sample->Ended) {
      slot.List->ResolveQueryData(slot.Heap, D3D12_QUERY_TYPE_TIMESTAMP,
                                  sample->Index, 2, slot.Readback,
                                  sample->Index * sizeof(UINT64));
    } else {
      LOG("D3D12 GPU scope was not completed in its recording frame; sample "
          "discarded",
          MGDF_LOG_ERROR);
    }
  }
  result = slot.List->Close();
  if (FAILED(result)) return result;
  ID3D12CommandList *lists[] = {slot.List};
  _queue->ExecuteCommandLists(1, lists);
  return S_OK;
}

void D3D12CounterManager::Remove(D3D12PerformanceCounter *counter) {
  std::lock_guard lock(_mutex);
  _counters.erase(counter);
}
void D3D12CounterManager::Snapshots(
    std::vector<CounterSnapshot> &snapshots) const {
  std::lock_guard lock(_mutex);
  for (auto counter : _counters)
    counter->Snapshot(snapshots.emplace_back(), true);
}

}  // namespace core
}  // namespace MGDF
