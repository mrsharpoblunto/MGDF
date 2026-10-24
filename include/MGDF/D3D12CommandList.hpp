#pragma once

#include <d3d12.h>

#include "ComObject.hpp"

namespace MGDF {

// Private data copied by D3D12; no pointers cross the module/host boundary.
inline constexpr GUID D3D12CommandListRecordingID{
    0xe59e8028,
    0xba6b,
    0x4cd4,
    {0x80, 0x6b, 0xf1, 0xca, 0x3b, 0xe5, 0x01, 0xc6}};
struct D3D12CommandListRecording {
  UINT64 Generation = 0;
  BOOL Open = FALSE;
};

/**
Owns a native command list used with MGDF GPU counters. Use Reset and Close on
this object, and Get for recording commands, creating counters and submission.
Do not call Reset or Close through the native pointer: D3D12 has no public query
for the recording state, so counters depend on this helper's private data.
*/
class D3D12CommandList {
 public:
  HRESULT Create(ID3D12Device *device, ID3D12CommandAllocator *allocator) {
    Clear();
    const auto result =
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator,
                                  nullptr, IID_PPV_ARGS(_list.Assign()));
    if (FAILED(result)) return result;
    return SetRecording(true);
  }
  HRESULT Reset(ID3D12CommandAllocator *allocator,
                ID3D12PipelineState *state = nullptr) {
    const auto result = _list->Reset(allocator, state);
    if (FAILED(result)) return result;
    return SetRecording(true);
  }
  HRESULT Close() {
    const auto state = SetRecording(false);
    if (FAILED(state)) return state;
    return _list->Close();
  }
  void Clear() {
    if (_list) SetRecording(false);
    _list.Clear();
  }
  ~D3D12CommandList() { Clear(); }
  D3D12CommandList() = default;
  D3D12CommandList(const D3D12CommandList &) = delete;
  D3D12CommandList &operator=(const D3D12CommandList &) = delete;
  ID3D12GraphicsCommandList *Get() const { return _list; }

 private:
  HRESULT SetRecording(bool open) {
    if (open) ++_recording.Generation;
    _recording.Open = open;
    return _list->SetPrivateData(D3D12CommandListRecordingID,
                                 sizeof(_recording), &_recording);
  }
  ComObject<ID3D12GraphicsCommandList> _list;
  D3D12CommandListRecording _recording{};
};

}  // namespace MGDF
