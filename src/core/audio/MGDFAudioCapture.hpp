#pragma once

#include <MGDF/ComObject.hpp>
#include <array>
#include <atomic>
#include <cstdint>

namespace MGDF::core::audio {

constexpr UINT32 MixRate = 48000;
constexpr UINT32 MixBlockFrames = 240;
constexpr int64_t AudioTicksPerSecond = 10'000'000;

struct AudioBlock {
  UINT64 FirstFrame = 0;
  int64_t PlayTime = 0;  // QPC in 100 ns units, as returned by IAudioClock.
  UINT32 Frames = 0;
  std::array<float, MixBlockFrames * 2> Samples{};
};

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6101"))
IAudioCaptureState : IUnknown {};

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6105"))
IAudioCaptureCounts : IUnknown {};
class AudioCaptureCounts : public ComBase<IAudioCaptureCounts> {
 public:
  std::atomic<UINT64> Dropped{0};
};

// One output producer and one recording worker. A full ring drops the copy.
class AudioCaptureRing : public ComBase<IAudioCaptureState> {
 public:
  explicit AudioCaptureRing(const ComObject<AudioCaptureCounts> &counts)
      : Ready(CreateEventW(nullptr, FALSE, FALSE, nullptr)), Counts(counts) {}
  ~AudioCaptureRing() {
    if (Ready) CloseHandle(Ready);
  }
  HANDLE Ready = nullptr;
  std::atomic<bool> Active{true};
  ComObject<AudioCaptureCounts> Counts;
  std::atomic<int64_t> Through{0};

  bool Push(const float *samples, UINT32 channels, UINT32 frames,
            UINT64 firstFrame, int64_t playTime) {
    _ASSERTE(frames <= MixBlockFrames);
    const auto write = _write.load(std::memory_order_relaxed);
    if (write - _read.load(std::memory_order_acquire) == _blocks.size()) {
      Counts->Dropped.fetch_add(1, std::memory_order_relaxed);
      Through.store(playTime + frames * AudioTicksPerSecond / MixRate,
                    std::memory_order_release);
      SetEvent(Ready);
      return false;
    }
    auto &block = _blocks[write % _blocks.size()];
    block.FirstFrame = firstFrame;
    block.PlayTime = playTime;
    block.Frames = frames;
    for (UINT32 i = 0; i < frames; ++i) {
      const auto *in = samples + size_t(i) * channels;
      float left = in[0], right = in[1];
      constexpr float surround = 0.7071067811865475f;
      if (channels == 4) {
        left += surround * in[2];
        right += surround * in[3];
      } else if (channels >= 6) {
        // ITU stereo downmix: center and surrounds at -3 dB, no LFE.
        left += surround * (in[2] + in[4]);
        right += surround * (in[2] + in[5]);
        if (channels == 8) {
          left += surround * in[6];
          right += surround * in[7];
        }
      }
      block.Samples[size_t(i) * 2] = left;
      block.Samples[size_t(i) * 2 + 1] = right;
    }
    _write.store(write + 1, std::memory_order_release);
    Through.store(playTime + frames * AudioTicksPerSecond / MixRate,
                  std::memory_order_release);
    SetEvent(Ready);
    return true;
  }
  bool Pop(AudioBlock &block) {
    const auto read = _read.load(std::memory_order_relaxed);
    if (read == _write.load(std::memory_order_acquire)) return false;
    block = _blocks[read % _blocks.size()];
    _read.store(read + 1, std::memory_order_release);
    return true;
  }

 private:
  std::array<AudioBlock, 128> _blocks{};
  std::atomic<UINT64> _read{0}, _write{0};
};

struct __declspec(uuid("AB51B6E4-5E38-44CA-B9BA-F2B3EF9F6102"))
IAudioCaptureSource : IUnknown {};

class AudioCaptureSource : public ComBase<IAudioCaptureSource> {
 public:
  ~AudioCaptureSource() {
    if (auto *pending = _pending.exchange(nullptr)) pending->Release();
  }
  ComObject<AudioCaptureRing> Start() {
    auto ring = MakeCom<AudioCaptureRing>(Counts);
    ring->AddRef();
    if (auto *pending = _pending.exchange(ring.Get())) pending->Release();
    return ring;
  }
  // Only the output thread calls Publish. No recorder lock or module call.
  void Publish(const float *samples, UINT32 channels, UINT32 frames,
               UINT64 firstFrame, int64_t playTime) {
    if (auto *pending = _pending.exchange(nullptr))
      _current = ComObject<AudioCaptureRing>(pending);
    if (_current && _current->Active.load(std::memory_order_acquire))
      _current->Push(samples, channels, frames, firstFrame, playTime);
  }
  ComObject<AudioCaptureCounts> Counts = MakeCom<AudioCaptureCounts>();

 private:
  std::atomic<AudioCaptureRing *> _pending{nullptr};
  ComObject<AudioCaptureRing> _current;
};
}  // namespace MGDF::core::audio
