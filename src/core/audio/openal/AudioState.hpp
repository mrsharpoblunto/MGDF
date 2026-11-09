#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace MGDF::core::audio::openal_audio {

static_assert(std::atomic<UINT64>::is_always_lock_free);
static_assert(std::atomic<unsigned>::is_always_lock_free);

// Readers pin a published slot. The writer skips a busy slot and never waits.
template <typename T>
class AudioSnapshot {
 public:
  T Read() const {
    for (;;) {
      const auto index = _current.load();
      auto &slot = _slots[index];
      slot.Readers.fetch_add(1);
      if (_current.load() == index) {
        const T value = slot.Value;
        slot.Readers.fetch_sub(1);
        return value;
      }
      slot.Readers.fetch_sub(1);
    }
  }
  void Publish(const T &value) {
    const auto current = _current.load();
    for (unsigned i = 0; i < _slots.size(); ++i) {
      if (i != current && _slots[i].Readers.load() == 0) {
        _slots[i].Value = value;
        _current.store(i);
        return;
      }
    }
  }

 private:
  struct Slot {
    mutable std::atomic<unsigned> Readers{0};
    T Value{};
  };
  mutable std::array<Slot, 3> _slots;
  std::atomic<unsigned> _current{0};
};

template <typename T>
class AudioQueue {
 public:
  ~AudioQueue() {
    auto *item = Close();
    while (item) {
      auto *next = item->Next;
      delete item;
      item = next;
    }
  }
  bool Push(T *item) {
    auto *head = _head.load();
    do {
      if (head == Closed()) return false;
      item->Next = head;
    } while (!_head.compare_exchange_weak(head, item));
    return true;
  }
  T *Take() {
    auto *head = _head.load();
    do {
      if (head == Closed()) return nullptr;
    } while (!_head.compare_exchange_weak(head, nullptr));
    return Reverse(head);
  }
  T *Close() {
    auto *head = _head.exchange(Closed());
    return head == Closed() ? nullptr : Reverse(head);
  }

 private:
  static T *Closed() { return &_closed; }
  inline static T _closed{};
  static T *Reverse(T *head) {
    T *result = nullptr;
    while (head) {
      auto *next = head->Next;
      head->Next = result;
      result = head;
      head = next;
    }
    return result;
  }
  std::atomic<T *> _head{nullptr};
};

struct AudioControl;
struct SampleSegment {
  std::vector<float> Samples;
  std::vector<float> Rms;
  bool Loop = false;
  double Duration = 0;
};
struct SampleData {
  enum State { Pending, Decoded, Loaded, Failed };
  std::atomic<State> Status{Pending};
  std::vector<MGDFSoundSegment> Descriptors;
  std::vector<SampleSegment> Segments;
  UINT32 Channels = 0, Rate = 0;
};

class SoundSamples : public ComBase<IMGDFSoundSamples> {
 public:
  SoundSamples(std::shared_ptr<AudioControl> owner,
               std::shared_ptr<SampleData> data)
      : Owner(std::move(owner)), Data(std::move(data)) {}
  BOOL __stdcall IsLoaded() final {
    return Data->Status.load() == SampleData::Loaded;
  }
  BOOL __stdcall HasFailed() final {
    return Data->Status.load() == SampleData::Failed;
  }
  UINT32 __stdcall GetSegmentCount() final {
    return static_cast<UINT32>(Data->Descriptors.size());
  }
  double __stdcall GetDuration(UINT32 segment) final {
    return IsLoaded() && segment < Data->Segments.size()
               ? Data->Segments[segment].Duration
               : 0;
  }
  const std::shared_ptr<AudioControl> Owner;
  const std::shared_ptr<SampleData> Data;
};

struct PlayPosition {
  UINT64 Frame = 0;
  double Time = 0;
};

class SoundPcmStream : public ComBase<IMGDFSoundPcmStream> {
 public:
  SoundPcmStream(std::shared_ptr<AudioControl> owner, MGDFSoundPcmDesc desc);
  UINT32 __stdcall GetChannels() final { return _desc.Channels; }
  UINT32 __stdcall GetSampleRate() final { return _desc.SampleRate; }
  UINT32 __stdcall GetWritableFrames() final;
  UINT32 __stdcall GetQueuedFrames() final;
  UINT32 __stdcall Write(const float *samples, UINT32 frames) final;
  UINT32 __stdcall WriteSilence(UINT32 frames) final;
  void __stdcall End() final { Ended.store(true, std::memory_order_release); }
  void __stdcall GetPlayPosition(UINT64 *frame, double *audioTime) final;
  UINT32 Consume(float *output, UINT32 frames);
  UINT64 ReadCount() const { return _read.load(std::memory_order_acquire); }
  void Publish(UINT64 frame, double time) { _position.Publish({frame, time}); }
  bool Claim(MGDFVoiceId id);
  void ReleaseVoice(bool stopped);
  const std::shared_ptr<AudioControl> Owner;
  std::atomic<bool> Ended{false};

 private:
  UINT32 Put(const float *samples, UINT32 frames);
  const MGDFSoundPcmDesc _desc;
  std::vector<float> _samples;
  std::vector<unsigned char> _silence;
  std::atomic<UINT64> _written{0};
  std::atomic<UINT64> _read{0};
  std::atomic<MGDFVoiceId> _voice{0};
  std::atomic<bool> _stopped{false};
  AudioSnapshot<PlayPosition> _position;
};

enum class AudioOp {
  Play,
  PlayPcm,
  Stop,
  Volume,
  Pitch,
  Audibility,
  GainHF,
  Spatial,
  BusVolume,
  BusMute,
  BusPaused,
  BusGainHF,
  Listener,
  Speed,
  Doppler
};
struct AudioCommand {
  AudioOp Op = AudioOp::Stop;
  MGDFVoiceId Voice = 0;
  MGDFVoiceDesc Desc{};
  ComObject<SoundSamples> Samples;
  ComObject<SoundPcmStream> Pcm;
  MGDFSoundListener Listener{};
  float Value = 0, Ramp = 0;
};
struct AudioBatch {
  AudioBatch *Next = nullptr;
  std::vector<AudioCommand> Commands;
};
struct AudioBusInfo {
  std::wstring Name;
  std::string Preference;
  MGDFBusId Parent = 0;
  float InitialVolume = 1;
  std::atomic<float> Volume{1};
};
struct MixerSnapshot {
  MGDFSoundStats Stats{};
  double Time = 0;
};
struct AudioControl {
  static constexpr size_t VoiceSlots = 65536;
  static constexpr size_t BusSlots = 256;
  AudioQueue<AudioBatch> Submissions;
  std::array<std::atomic<UINT64>, VoiceSlots> Status{};
  std::array<std::atomic<AudioBusInfo *>, BusSlots> Buses{};
  std::atomic<UINT64> NextVoice{1};
  std::atomic<UINT32> NextBus{3};
  std::atomic<UINT32> SampleRate{0};
  AudioSnapshot<MixerSnapshot> Snapshot;
  ~AudioControl() {
    for (auto &bus : Buses) delete bus.load();
  }
  MGDFVoiceId NewVoice();
  void SetStatus(MGDFVoiceId id, MGDFVoiceStatus status);
  MGDFVoiceStatus GetStatus(MGDFVoiceId id) const;
};

class SoundCommandList : public ComBase<IMGDFSoundCommandList> {
 public:
  explicit SoundCommandList(std::shared_ptr<AudioControl> owner)
      : Owner(std::move(owner)) {}
  ~SoundCommandList();
  MGDFVoiceId __stdcall Play(IMGDFSoundSamples *, const MGDFVoiceDesc *) final;
  MGDFVoiceId __stdcall PlayPcm(IMGDFSoundPcmStream *,
                                const MGDFVoiceDesc *) final;
  void __stdcall Stop(MGDFVoiceId, float) final;
  void __stdcall SetVolume(MGDFVoiceId, float, float) final;
  void __stdcall SetPitch(MGDFVoiceId, float, float) final;
  void __stdcall SetAudibility(MGDFVoiceId, float) final;
  void __stdcall SetGainHF(MGDFVoiceId, float, float) final;
  void __stdcall SetSpatial(MGDFVoiceId, const MGDFSoundPosition *,
                            const MGDFSoundPosition *, float, float) final;
  void __stdcall SetBusVolume(MGDFBusId, float, float) final;
  void __stdcall SetBusMute(MGDFBusId, BOOL) final;
  void __stdcall SetBusPaused(MGDFBusId, BOOL) final;
  void __stdcall SetBusGainHF(MGDFBusId, float, float) final;
  void __stdcall SetListener(const MGDFSoundListener *) final;
  bool Add(AudioCommand command) noexcept;
  AudioBatch *Detach() { return _batch.release(); }
  const std::shared_ptr<AudioControl> Owner;

 private:
  void Scalar(AudioOp op, MGDFVoiceId id, float value, float ramp);
  std::unique_ptr<AudioBatch> _batch;
};

struct DecodeJob {
  DecodeJob *Next = nullptr;
  ComObject<IMGDFReadOnlyFile> File;
  std::shared_ptr<SampleData> Data;
};
void DecodeSamples(const DecodeJob &job);
bool ValidVoice(const MGDFVoiceDesc &desc, const AudioControl &control);

}  // namespace MGDF::core::audio::openal_audio
