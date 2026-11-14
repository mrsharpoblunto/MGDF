#include "Stdafx.h"
#undef min
#undef max

#include <algorithm>
#include <cmath>

#include "AudioState.hpp"

namespace MGDF::core::audio::openal_audio {

SoundSamples::SoundSamples(ComObject<AudioControl> owner,
                           ComObject<SampleData> data)
    : Owner(std::move(owner)),
      Data(std::move(data)),
      _retirement(std::make_unique<SampleRetirement>()) {}
SoundSamples::~SoundSamples() {
  // Transfer the last sample reference to the audio thread without allocation.
  _retirement->Data = std::move(Data);
  if (Owner->RetiredSamples.Push(_retirement.get())) _retirement.release();
}

SoundPcmStream::SoundPcmStream(ComObject<AudioControl> owner,
                               MGDFSoundPcmDesc desc)
    : Owner(std::move(owner)),
      _desc(desc),
      _samples(static_cast<size_t>(desc.CapacityFrames) * desc.Channels),
      _silence(desc.CapacityFrames) {}

UINT32 SoundPcmStream::GetQueuedFrames() {
  const auto read = _read.load(std::memory_order_acquire);
  const auto written = _written.load(std::memory_order_acquire);
  return static_cast<UINT32>(
      std::min<UINT64>(written - read, _desc.CapacityFrames));
}
UINT32 SoundPcmStream::GetWritableFrames() {
  return Ended.load() || _stopped.load()
             ? 0
             : _desc.CapacityFrames - GetQueuedFrames();
}
UINT32 SoundPcmStream::Write(const float *samples, UINT32 frames) {
  return samples ? Put(samples, frames) : 0;
}
UINT32 SoundPcmStream::WriteSilence(UINT32 frames) {
  return Put(nullptr, frames);
}
UINT32 SoundPcmStream::Put(const float *samples, UINT32 frames) {
  if (Ended.load(std::memory_order_acquire) || _stopped.load()) return 0;
  const auto written = _written.load(std::memory_order_relaxed);
  const auto read = _read.load(std::memory_order_acquire);
  const auto count = std::min(
      frames, _desc.CapacityFrames - static_cast<UINT32>(written - read));
  for (UINT32 i = 0; i < count; ++i) {
    const auto index = (written + i) % _desc.CapacityFrames;
    _silence[index] = samples ? 0 : 1;
    if (samples) {
      for (UINT32 c = 0; c < _desc.Channels; ++c)
        _samples[index * _desc.Channels + c] =
            samples[static_cast<size_t>(i) * _desc.Channels + c];
    }
  }
  _written.store(written + count, std::memory_order_release);
  return count;
}
UINT32 SoundPcmStream::Consume(float *output, UINT32 frames) {
  const auto read = _read.load(std::memory_order_relaxed);
  const auto written = _written.load(std::memory_order_acquire);
  const auto count =
      static_cast<UINT32>(std::min<UINT64>(frames, written - read));
  if (output) {
    for (UINT32 i = 0; i < count; ++i) {
      const auto index = (read + i) % _desc.CapacityFrames;
      for (UINT32 c = 0; c < _desc.Channels; ++c)
        output[static_cast<size_t>(i) * _desc.Channels + c] =
            _silence[index] ? 0 : _samples[index * _desc.Channels + c];
    }
  }
  _read.store(read + count, std::memory_order_release);
  return count;
}
void SoundPcmStream::GetPlayPosition(UINT64 *frame, double *audioTime) {
  const auto position = _position.Read();
  if (frame) *frame = position.Frame;
  if (audioTime) *audioTime = position.Time;
}
bool SoundPcmStream::Claim(MGDFVoiceId id) {
  MGDFVoiceId empty = 0;
  if (!_voice.compare_exchange_strong(empty, id)) return false;
  _stopped.store(false);
  return true;
}
void SoundPcmStream::ReleaseVoice(bool stopped) {
  if (stopped) _stopped.store(true);
  _voice.store(0);
}

MGDFVoiceId AudioControl::NewVoice() {
  const auto id = NextVoice.fetch_add(1);
  auto &slot = Status[id % VoiceSlots];
  auto previous = slot.load();
  if (previous && (previous & 7) != MGDF_VOICE_ENDED) return 0;
  return slot.compare_exchange_strong(previous, (id << 3) | MGDF_VOICE_PENDING)
             ? id
             : 0;
}
void AudioControl::SetStatus(MGDFVoiceId id, MGDFVoiceStatus status) {
  auto &slot = Status[id % VoiceSlots];
  auto previous = slot.load();
  if ((previous >> 3) == id)
    slot.compare_exchange_strong(previous, (id << 3) | status);
}
MGDFVoiceStatus AudioControl::GetStatus(MGDFVoiceId id) const {
  const auto value = Status[id % VoiceSlots].load();
  return id && (value >> 3) == id ? static_cast<MGDFVoiceStatus>(value & 7)
                                  : MGDF_VOICE_ENDED;
}
bool ValidVoice(const MGDFVoiceDesc &d, const AudioControl &control) {
  const auto finite = [](MGDFSoundPosition p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
  };
  return d.Bus < AudioControl::BusSlots && control.Buses[d.Bus].load() &&
         std::isfinite(d.Volume) && d.Volume >= 0 && std::isfinite(d.Pitch) &&
         d.Pitch > 0 && d.Pitch <= 16 && std::isfinite(d.StartTime) &&
         d.StartTime >= 0 && std::isfinite(d.StartOffset) &&
         d.StartOffset >= 0 && std::isfinite(d.FadeIn) && d.FadeIn >= 0 &&
         std::isfinite(d.Audibility) && d.Audibility >= 0 &&
         std::isfinite(d.Radius) && d.Radius >= 0 && std::isfinite(d.GainHF) &&
         d.GainHF >= 0 && d.GainHF <= 1 && finite(d.Position) &&
         finite(d.Velocity);
}
SoundCommandList::~SoundCommandList() {
  if (!_batch) return;
  for (auto &command : _batch->Commands) {
    if (command.Op == AudioOp::Play || command.Op == AudioOp::PlayPcm) {
      Owner->SetStatus(command.Voice, MGDF_VOICE_ENDED);
      if (command.Pcm) command.Pcm->ReleaseVoice(false);
    }
  }
}
bool SoundCommandList::Add(AudioCommand command) noexcept {
  try {
    if (!_batch) _batch = std::make_unique<AudioBatch>();
    _batch->Commands.push_back(std::move(command));
    return true;
  } catch (...) {
    return false;
  }
}
MGDFVoiceId SoundCommandList::Play(IMGDFSoundSamples *samples,
                                   const MGDFVoiceDesc *desc) {
  auto *value = dynamic_cast<SoundSamples *>(samples);
  if (!value || value->Owner != Owner || !desc ||
      !ValidVoice(*desc, *Owner.Get()) ||
      desc->Segment >= value->GetSegmentCount() ||
      (desc->Spatial && value->IsLoaded() && value->Data->Channels != 1))
    return 0;
  const auto id = Owner->NewVoice();
  if (id) {
    AudioCommand command;
    command.Op = AudioOp::Play;
    command.Voice = id;
    command.Desc = *desc;
    command.Samples = MakeComFromPtr<SoundSamples>(value);
    if (!Add(std::move(command))) {
      Owner->SetStatus(id, MGDF_VOICE_ENDED);
      return 0;
    }
  }
  return id;
}
MGDFVoiceId SoundCommandList::PlayPcm(IMGDFSoundPcmStream *stream,
                                      const MGDFVoiceDesc *desc) {
  if (!desc) return 0;
  auto pcmDesc = *desc;
  pcmDesc.Segment = 0;
  pcmDesc.StartOffset = 0;
  auto *value = dynamic_cast<SoundPcmStream *>(stream);
  if (!value || value->Owner != Owner || !ValidVoice(pcmDesc, *Owner.Get()) ||
      (desc->Spatial && value->GetChannels() != 1))
    return 0;
  const auto id = Owner->NewVoice();
  if (!id) return 0;
  if (!value->Claim(id)) {
    Owner->SetStatus(id, MGDF_VOICE_ENDED);
    return 0;
  }
  AudioCommand command;
  command.Op = AudioOp::PlayPcm;
  command.Voice = id;
  command.Desc = *desc;
  command.Pcm = MakeComFromPtr<SoundPcmStream>(value);
  if (!Add(std::move(command))) {
    value->ReleaseVoice(false);
    Owner->SetStatus(id, MGDF_VOICE_ENDED);
    return 0;
  }
  return id;
}
void SoundCommandList::Scalar(AudioOp op, MGDFVoiceId id, float value,
                              float ramp) {
  if (!std::isfinite(value) || !std::isfinite(ramp) || ramp < 0 || value < 0)
    return;
  AudioCommand command;
  command.Op = op;
  command.Voice = id;
  command.Value = value;
  command.Ramp = ramp;
  Add(std::move(command));
}
void SoundCommandList::Stop(MGDFVoiceId id, float fade) {
  Scalar(AudioOp::Stop, id, 0, fade);
}
void SoundCommandList::SetVolume(MGDFVoiceId id, float value, float ramp) {
  Scalar(AudioOp::Volume, id, value, ramp);
}
void SoundCommandList::SetPitch(MGDFVoiceId id, float value, float ramp) {
  if (value > 0 && value <= 16) Scalar(AudioOp::Pitch, id, value, ramp);
}
void SoundCommandList::SetAudibility(MGDFVoiceId id, float value) {
  Scalar(AudioOp::Audibility, id, value, 0);
}
void SoundCommandList::SetGainHF(MGDFVoiceId id, float value, float ramp) {
  if (value <= 1) Scalar(AudioOp::GainHF, id, value, ramp);
}
void SoundCommandList::SetSpatial(MGDFVoiceId id,
                                  const MGDFSoundPosition *position,
                                  const MGDFSoundPosition *velocity,
                                  float radius, float ramp) {
  if (!position || !velocity) return;
  AudioCommand command;
  command.Op = AudioOp::Spatial;
  command.Voice = id;
  command.Ramp = ramp;
  command.Desc.Position = *position;
  command.Desc.Velocity = *velocity;
  command.Desc.Radius = radius;
  if (!std::isfinite(ramp) || ramp < 0 || !std::isfinite(radius) || radius < 0)
    return;
  const float values[]{position->x, position->y, position->z,
                       velocity->x, velocity->y, velocity->z};
  for (const float value : values)
    if (!std::isfinite(value)) return;
  Add(std::move(command));
}
void SoundCommandList::SetBusVolume(MGDFBusId id, float value, float ramp) {
  Scalar(AudioOp::BusVolume, id, value, ramp);
}
void SoundCommandList::SetBusMute(MGDFBusId id, BOOL value) {
  Scalar(AudioOp::BusMute, id, value ? 1.0f : 0.0f, 0);
}
void SoundCommandList::SetBusPaused(MGDFBusId id, BOOL value) {
  Scalar(AudioOp::BusPaused, id, value ? 1.0f : 0.0f, 0);
}
void SoundCommandList::SetBusGainHF(MGDFBusId id, float value, float ramp) {
  if (value <= 1) Scalar(AudioOp::BusGainHF, id, value, ramp);
}
void SoundCommandList::SetListener(const MGDFSoundListener *value) {
  if (!value) return;
  const float values[]{value->Position.x, value->Position.y, value->Position.z,
                       value->Velocity.x, value->Velocity.y, value->Velocity.z,
                       value->Forward.x,  value->Forward.y,  value->Forward.z,
                       value->Up.x,       value->Up.y,       value->Up.z};
  for (const float component : values)
    if (!std::isfinite(component)) return;
  AudioCommand command;
  command.Op = AudioOp::Listener;
  command.Listener = *value;
  Add(std::move(command));
}

}  // namespace MGDF::core::audio::openal_audio
