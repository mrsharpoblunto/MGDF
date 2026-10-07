#pragma once

#include <memory>

#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

class D3D12CounterManager;

class D3D12PerformanceCounter : public CounterBase {
 public:
  D3D12PerformanceCounter(IMGDFMetric *metric, Timer &timer,
                          D3D12CounterManager &manager,
                          ID3D12GraphicsCommandList *list);
  ~D3D12PerformanceCounter();
  void Record(double value, const std::map<std::string, std::string> &tags);
  void Reset() { _list.Clear(); }
  ID3D12GraphicsCommandList *List() const { return _list; }

 protected:
  HRESULT DoBegin(std::map<std::string, std::string> &tags,
                  IMGDFPerformanceCounterScope **scope) final;

 private:
  D3D12CounterManager &_manager;
  ComObject<ID3D12GraphicsCommandList> _list;
};

class D3D12CounterManager {
 public:
  static constexpr UINT MaxPairs = 256;
  struct Sample {
    ComObject<D3D12PerformanceCounter> Counter;
    std::map<std::string, std::string> Tags;
    UINT64 Generation = 0;
    UINT64 Frame = 0;
    UINT Index = 0;
    bool Ended = false;
  };

  explicit D3D12CounterManager(Timer &timer) : _timer(timer) {}
  ~D3D12CounterManager() { Reset(); }
  HRESULT Init(ID3D12Device10 *device, ID3D12CommandQueue *queue, UINT slots);
  void Reset();
  HRESULT Create(IMGDFMetric *metric, ID3D12GraphicsCommandList *list,
                 IMGDFPerformanceCounter **counter);
  HRESULT Begin(D3D12PerformanceCounter *counter,
                std::map<std::string, std::string> &tags,
                IMGDFPerformanceCounterScope **scope);
  void End(const std::shared_ptr<Sample> &sample);
  void BeginFrame(const MGDFFrameInfo &frame);
  HRESULT EndFrame();
  void Remove(D3D12PerformanceCounter *counter);
  void Snapshots(std::vector<CounterSnapshot> &snapshots) const;

 private:
  struct Slot {
    ComObject<ID3D12QueryHeap> Heap;
    ComObject<ID3D12Resource> Readback;
    ComObject<ID3D12CommandAllocator> Allocator;
    ComObject<ID3D12GraphicsCommandList> List;
    const UINT64 *Results = nullptr;
    std::vector<std::shared_ptr<Sample>> Pending;
    UINT64 Fence = 0;
  };
  Timer &_timer;
  ComObject<ID3D12Device10> _device;
  ComObject<ID3D12CommandQueue> _queue;
  std::vector<std::unique_ptr<Slot>> _slots;
  UINT64 _frequency = 0;
  MGDFFrameInfo _frame{};
  bool _recording = false;
  std::mutex _frameMutex;
  mutable std::mutex _mutex;
  std::set<D3D12PerformanceCounter *> _counters;
};

}  // namespace core
}  // namespace MGDF
