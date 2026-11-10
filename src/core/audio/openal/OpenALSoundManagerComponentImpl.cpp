#include "Stdafx.h"
#undef min
#undef max
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "../../common/MGDFPreferenceSet.hpp"
#include "AudioState.hpp"
#include "OpenALSoundManagerComponentImpl.hpp"
#include "SoundStream.hpp"

namespace MGDF::core::audio::openal_audio {
namespace {
struct Ramp {
  float From = 1, To = 1;
  double Begin = 0, End = 0;
  float At(double time) const {
    if (time >= End || End <= Begin) return To;
    if (time <= Begin) return From;
    return From +
           (To - From) * static_cast<float>((time - Begin) / (End - Begin));
  }
  void Set(float value, double now, float duration = 0) {
    From = At(now);
    To = value;
    Begin = now;
    End = now + duration;
  }
  double Integral(double first, double last) const {
    if (last <= first) return 0;
    double result = 0;
    if (first < Begin) {
      const auto end = std::min(last, Begin);
      result += (end - first) * From;
      first = end;
    }
    if (first < End) {
      const auto end = std::min(last, End);
      result +=
          (end - first) * (static_cast<double>(At(first)) + At(end)) * 0.5;
      first = end;
    }
    return result + (last - first) * To;
  }
};
struct Bus {
  Ramp Volume, GainHF;
  bool Initialized = false, Mute = false, Pause = false, Paused = false;
  float Gain = 1, HF = 1;
};
struct CallbackSpan {
  std::atomic<UINT64> Version{0}, Begin{0}, DataBegin{0}, DataCount{0},
      Length{0};
};
struct PcmFeed {
  SoundPcmStream *Stream = nullptr;
  UINT64 Base = 0, Offset = 0, Serial = 0;
  std::array<CallbackSpan, 256> History;
  std::atomic<UINT64> Latest{0}, Underruns{0};
  static ALsizei AL_APIENTRY Read(void *user, void *output,
                                  ALsizei bytes) noexcept {
    auto &feed = *static_cast<PcmFeed *>(user);
    const auto channels = feed.Stream->GetChannels();
    const auto requested = static_cast<UINT32>(bytes) /
                           (channels * static_cast<UINT32>(sizeof(float)));
    const auto begin = feed.Stream->ReadCount();
    // Observe End before the ring so its final write is visible.
    const bool ended = feed.Stream->Ended.load(std::memory_order_acquire);
    const auto count =
        feed.Stream->Consume(static_cast<float *>(output), requested);
    const auto length = ended ? count : requested;
    if (!ended && count < requested) {
      std::fill_n(
          static_cast<float *>(output) + static_cast<size_t>(count) * channels,
          static_cast<size_t>(requested - count) * channels, 0.0f);
      feed.Underruns.fetch_add(1, std::memory_order_relaxed);
    }
    const auto serial = ++feed.Serial;
    auto &span = feed.History[serial % feed.History.size()];
    span.Version.store(serial * 2 + 1);
    span.Begin.store(feed.Offset);
    span.DataBegin.store(begin);
    span.DataCount.store(count);
    span.Length.store(length);
    span.Version.store(serial * 2 + 2);
    feed.Latest.store(serial);
    feed.Offset += length;
    return static_cast<ALsizei>(length * channels * sizeof(float));
  }
  UINT64 Position(UINT64 offset, UINT64 previous) const {
    const auto latest = Latest.load();
    for (UINT64 age = 0; age < std::min<UINT64>(latest, History.size());
         ++age) {
      const auto serial = latest - age;
      const auto &span = History[serial % History.size()];
      const auto version = span.Version.load();
      const auto begin = span.Begin.load(), data = span.DataBegin.load();
      const auto count = span.DataCount.load(), length = span.Length.load();
      if (version != serial * 2 + 2 || span.Version.load() != version) continue;
      if (offset >= begin && offset <= begin + length)
        return data + std::min(offset - begin, count);
    }
    return offset == 0 ? Base : previous;
  }
};
struct Voice {
  MGDFVoiceId Id = 0;
  MGDFVoiceDesc Desc{};
  ComObject<SoundSamples> Samples;
  ComObject<SoundPcmStream> Pcm;
  Ramp Volume, Pitch, HF, Fade;
  std::array<Ramp, 7> Spatial;
  double LastTime = 0, Position = 0, Fraction = 0;
  UINT64 Played = 0;
  int Source = -1;
  bool Stopping = false, Ended = false, Paused = false, Waiting = false;
  bool OwnPause = false, Pausing = false, FileStream = false;
  std::unique_ptr<PcmFeed> Feed;
};
struct Source {
  ALuint Id = 0, Filter = 0, Callback = 0;
};
struct SampleRegistration {
  SampleRegistration *Next = nullptr;
  ComObject<SampleData> Data;
};
struct SampleBuffers {
  ComObject<SampleData> Data;
  std::vector<ALuint> Buffers;
};
void Discard(AudioBatch *batch, AudioControl &control) {
  while (batch) {
    auto *next = batch->Next;
    for (auto &command : batch->Commands) {
      if (command.Op == AudioOp::Play || command.Op == AudioOp::PlayPcm) {
        control.SetStatus(command.Voice, MGDF_VOICE_ENDED);
        if (command.Pcm) command.Pcm->ReleaseVoice(false);
      }
    }
    delete batch;
    batch = next;
  }
}
}  // namespace

struct OpenALSoundManagerComponentImpl::Impl {
  AudioOptions Options;
  ComObject<AudioControl> Control = MakeCom<AudioControl>();
  std::atomic<bool> Stopping{false};
  std::thread AudioThread, Decoder;
  ComObject<AudioCaptureSource> Capture = MakeCom<AudioCaptureSource>();
  std::unique_ptr<AudioOutput> Output;
  std::unique_ptr<StreamWorker> Streams;
  AudioQueue<DecodeJob> Decodes;
  AudioQueue<SampleRegistration> Registrations;
  std::atomic<UINT32> Outstanding{0};
  std::mutex DecodeMutex;
  std::condition_variable DecodeComplete;
  ALCdevice *Device = nullptr;
  ALCcontext *Context = nullptr;
  LPALCRENDERSAMPLESSOFT RenderSamples = nullptr;
  LPALCGETINTEGER64VSOFT GetClock = nullptr;
  LPALGETSOURCEI64VSOFT GetOffset = nullptr;
  LPALSOURCEPLAYATTIMEVSOFT PlayAt = nullptr;
  LPALBUFFERCALLBACKSOFT BufferCallback = nullptr;
  LPALGENFILTERS GenFilters = nullptr;
  LPALDELETEFILTERS DeleteFilters = nullptr;
  LPALFILTERI Filteri = nullptr;
  LPALFILTERF Filterf = nullptr;
  std::vector<Source> Sources;
  std::vector<int> FreeSources;
  std::unordered_map<MGDFVoiceId, std::unique_ptr<Voice>> Voices;
  std::unordered_map<SampleData *, SampleBuffers> Samples;
  std::array<Bus, AudioControl::BusSlots> Buses;
  UINT64 Underruns = 0;
  double Now = 0;

  explicit Impl(AudioOptions options) : Options(options) {
    const wchar_t *const names[]{L"Master", L"Sound", L"Music"};
    const char *const preferences[]{"host.masterVolume", "host.soundVolume",
                                    "host.musicVolume"};
    for (size_t i = 0; i < 3; ++i) {
      auto bus = std::make_unique<AudioBusInfo>();
      bus->Name = names[i];
      bus->Preference = preferences[i];
      Control->Buses[i].store(bus.release());
    }
  }
  double Clock() const {
    ALCint64SOFT time = 0;
    GetClock(Device, ALC_DEVICE_CLOCK_SOFT, 1, &time);
    const double seconds = static_cast<double>(time) / 1e9;
    return Options.Loopback
               ? std::round(seconds * Options.SampleRate) / Options.SampleRate
               : seconds;
  }
  HRESULT Open() {
    const auto open = std::bit_cast<LPALCLOOPBACKOPENDEVICESOFT>(
        alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
    RenderSamples = std::bit_cast<LPALCRENDERSAMPLESSOFT>(
        alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));
    if (!open || !RenderSamples) return E_FAIL;
    Device = open(nullptr);
    if (!Device) return E_FAIL;
    std::vector<ALCint> attributes{
        ALC_MONO_SOURCES,        512,
        ALC_MAX_AUXILIARY_SENDS, 2,
        ALC_HRTF_SOFT,           Options.Hrtf ? ALC_TRUE : ALC_FALSE};
    attributes.insert(
        attributes.end(),
        {ALC_FREQUENCY, static_cast<ALCint>(Options.SampleRate),
         ALC_FORMAT_CHANNELS_SOFT, ALC_STEREO_SOFT, ALC_FORMAT_TYPE_SOFT,
         ALC_FLOAT_SOFT, ALC_OUTPUT_LIMITER_SOFT, ALC_FALSE,
         ALC_OUTPUT_MODE_SOFT,
         Options.Loopback || !Options.Hrtf ? ALC_STEREO_BASIC_SOFT
                                           : ALC_STEREO_HRTF_SOFT});
    attributes.push_back(0);
    Context = alcCreateContext(Device, attributes.data());
    if (!Context) return E_FAIL;
    const auto bind = std::bit_cast<PFNALCSETTHREADCONTEXTPROC>(
        alcGetProcAddress(Device, "alcSetThreadContext"));
    if (!bind || !bind(Context)) return E_FAIL;
    if (!alIsExtensionPresent("AL_SOFT_callback_buffer") ||
        !alIsExtensionPresent("AL_SOFT_source_start_delay") ||
        !alIsExtensionPresent("AL_EXT_SOURCE_RADIUS") ||
        !alcIsExtensionPresent(Device, "ALC_EXT_EFX") ||
        !alcIsExtensionPresent(Device, "ALC_SOFT_device_clock"))
      return E_FAIL;
    GetClock = std::bit_cast<LPALCGETINTEGER64VSOFT>(
        alcGetProcAddress(Device, "alcGetInteger64vSOFT"));
    GetOffset = std::bit_cast<LPALGETSOURCEI64VSOFT>(
        alGetProcAddress("alGetSourcei64vSOFT"));
    PlayAt = std::bit_cast<LPALSOURCEPLAYATTIMEVSOFT>(
        alGetProcAddress("alSourcePlayAtTimevSOFT"));
    BufferCallback = std::bit_cast<LPALBUFFERCALLBACKSOFT>(
        alGetProcAddress("alBufferCallbackSOFT"));
    GenFilters =
        std::bit_cast<LPALGENFILTERS>(alGetProcAddress("alGenFilters"));
    DeleteFilters =
        std::bit_cast<LPALDELETEFILTERS>(alGetProcAddress("alDeleteFilters"));
    Filteri = std::bit_cast<LPALFILTERI>(alGetProcAddress("alFilteri"));
    Filterf = std::bit_cast<LPALFILTERF>(alGetProcAddress("alFilterf"));
    if (!GetClock || !GetOffset || !PlayAt || !BufferCallback || !GenFilters ||
        !DeleteFilters || !Filteri || !Filterf)
      return E_FAIL;
    ALCint rate = 0;
    alcGetIntegerv(Device, ALC_FREQUENCY, 1, &rate);
    Control->SampleRate.store(static_cast<UINT32>(rate));
    alDistanceModel(AL_NONE);
    for (UINT32 i = 0; i < Options.Sources; ++i) {
      Source source;
      alGenSources(1, &source.Id);
      if (alGetError() != AL_NO_ERROR) break;
      GenFilters(1, &source.Filter);
      alGenBuffers(1, &source.Callback);
      Filteri(source.Filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
      Sources.push_back(source);
      FreeSources.push_back(static_cast<int>(i));
    }
    if (Sources.empty() || alGetError() != AL_NO_ERROR) return E_FAIL;
    Now = Clock();
    RefreshBuses();
    Publish(0);
    return S_OK;
  }
  void RefreshBuses() {
    for (size_t i = 0; i < Buses.size(); ++i) {
      auto *info = Control->Buses[i].load();
      if (!info) continue;
      auto &bus = Buses[i];
      if (!bus.Initialized) {
        bus.Volume.Set(info->InitialVolume, Now);
        bus.Initialized = true;
      }
      bus.Gain = bus.Mute ? 0 : bus.Volume.At(Now);
      bus.HF = bus.GainHF.At(Now);
      bus.Paused = bus.Pause;
      if (i) {
        bus.Gain *= Buses[info->Parent].Gain;
        bus.HF *= Buses[info->Parent].HF;
        bus.Paused |= Buses[info->Parent].Paused;
      }
      info->Volume.store(bus.Volume.At(Now));
    }
  }
  void UploadSamples() {
    // Each retired set must have its registration in this update.
    auto *retired = Control->RetiredSamples.Take();
    auto *registration = Registrations.Take();
    while (registration) {
      auto *next = registration->Next;
      auto data = std::move(registration->Data);
      Samples.emplace(data.Get(), SampleBuffers{data, {}});
      delete registration;
      registration = next;
    }
    while (retired) {
      std::unique_ptr<SampleRetirement> current(retired);
      retired = retired->Next;
      const auto it = Samples.find(current->Data.Get());
      if (it != Samples.end()) {
        const auto &buffers = it->second.Buffers;
        if (!buffers.empty())
          alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
        Samples.erase(it);
      }
    }
    for (auto &[key, record] : Samples) {
      auto &data = *record.Data.Get();
      if (data.Status.load(std::memory_order_acquire) == SampleData::Decoded) {
        record.Buffers.resize(data.Segments.size());
        alGenBuffers(static_cast<ALsizei>(record.Buffers.size()),
                     record.Buffers.data());
        for (size_t i = 0; i < record.Buffers.size(); ++i) {
          const auto &pcm = data.Segments[i].Samples;
          alBufferData(record.Buffers[i],
                       data.Channels == 1 ? AL_FORMAT_MONO_FLOAT32
                                          : AL_FORMAT_STEREO_FLOAT32,
                       pcm.data(),
                       static_cast<ALsizei>(pcm.size() * sizeof(float)),
                       static_cast<ALsizei>(data.Rate));
        }
        data.Status.store(alGetError() == AL_NO_ERROR ? SampleData::Loaded
                                                      : SampleData::Failed,
                          std::memory_order_release);
      }
    }
  }
  void ReleaseSource(Voice &voice) {
    if (voice.Source < 0) return;
    auto &source = Sources[voice.Source];
    alSourceStop(source.Id);
    alSourcei(source.Id, AL_BUFFER, 0);
    alSourcei(source.Id, AL_DIRECT_FILTER, AL_FILTER_NULL);
    if (voice.Pcm && voice.Feed) {
      voice.Played = voice.Pcm->ReadCount();
      voice.Pcm->Publish(voice.Played, Now);
      Underruns += voice.Feed->Underruns.load();
      voice.Feed.reset();
    }
    FreeSources.push_back(voice.Source);
    voice.Source = -1;
  }
  void Finish(Voice &voice, bool stopped) {
    ReleaseSource(voice);
    if (voice.Pcm) voice.Pcm->ReleaseVoice(stopped);
    voice.Ended = true;
    Control->SetStatus(voice.Id, MGDF_VOICE_ENDED);
  }
  void Advance(Voice &voice) {
    if (voice.Waiting && voice.Samples->IsLoaded()) {
      voice.Waiting = false;
      voice.Desc.StartTime = Now;
      voice.LastTime = Now;
      voice.Fade.From = voice.Fade.To = voice.Desc.FadeIn > 0 ? 0.0f : 1.0f;
      voice.Fade.Set(1, Now, voice.Desc.FadeIn);
    }
    const auto first = std::max(voice.LastTime, voice.Desc.StartTime);
    const auto elapsed =
        voice.Paused || voice.Waiting ? 0 : voice.Pitch.Integral(first, Now);
    voice.LastTime = Now;
    if (voice.Stopping && Now >= voice.Fade.End) {
      Finish(voice, true);
      return;
    }
    if (voice.Samples && voice.Samples->HasFailed()) {
      Finish(voice, false);
      return;
    }
    if (voice.Source >= 0) {
      const auto source = Sources[voice.Source].Id;
      ALint64SOFT offset[2]{};
      GetOffset(source, AL_SAMPLE_OFFSET_CLOCK_SOFT, offset);
      ALint state = AL_INITIAL;
      alGetSourcei(source, AL_SOURCE_STATE, &state);
      if (voice.Pcm) {
        if (state == AL_STOPPED)
          voice.Played = voice.Pcm->ReadCount();
        else if (offset[0] >= 0)
          voice.Played = voice.Feed->Position(
              static_cast<UINT64>(offset[0]) >> 32, voice.Played);
        voice.Pcm->Publish(voice.Played,
                           std::max(voice.Desc.StartTime,
                                    static_cast<double>(offset[1]) / 1e9));
        if (voice.Pcm->Ended.load() && !voice.Pcm->GetQueuedFrames() &&
            voice.Played == voice.Pcm->ReadCount()) {
          Finish(voice, false);
          return;
        }
      } else if (state == AL_STOPPED) {
        Finish(voice, false);
        return;
      } else if (offset[0] >= 0)
        voice.Position = static_cast<double>(offset[0]) / 4294967296.0 /
                         voice.Samples->Data->Rate;
    } else if (voice.Pcm) {
      const auto advance =
          voice.Fraction + elapsed * voice.Pcm->GetSampleRate();
      const auto frames =
          static_cast<UINT64>(static_cast<INT64>(std::floor(advance + 1e-9)));
      voice.Fraction = std::max(0.0, advance - static_cast<double>(frames));
      const auto requested =
          static_cast<UINT32>(std::min<UINT64>(frames, UINT32_MAX));
      const auto consumed = voice.Pcm->Consume(nullptr, requested);
      if (consumed < frames && !voice.Pcm->Ended.load()) ++Underruns;
      voice.Played = voice.Pcm->ReadCount();
      voice.Pcm->Publish(voice.Played, std::max(Now, voice.Desc.StartTime));
      if (Now >= voice.Desc.StartTime && voice.Pcm->Ended.load() &&
          !voice.Pcm->GetQueuedFrames()) {
        Finish(voice, false);
        return;
      }
    } else {
      voice.Position += elapsed;
      if (voice.Samples->IsLoaded()) {
        const auto &segment = voice.Samples->Data->Segments[voice.Desc.Segment];
        if (segment.Loop)
          voice.Position = std::fmod(voice.Position, segment.Duration);
        else if (voice.Position >= segment.Duration) {
          Finish(voice, false);
          return;
        }
      }
    }
  }
  void Apply(AudioCommand &command) {
    const auto id = command.Voice;
    if (command.Op == AudioOp::Play || command.Op == AudioOp::PlayPcm) {
      auto voice = std::make_unique<Voice>();
      voice->Id = id;
      voice->Desc = command.Desc;
      voice->FileStream = command.FileStream;
      voice->Samples = std::move(command.Samples);
      voice->Pcm = std::move(command.Pcm);
      voice->Desc.StartTime =
          command.Desc.StartTime ? command.Desc.StartTime : Now;
      voice->LastTime = voice->Desc.StartTime;
      voice->Waiting = voice->Samples && !voice->Samples->IsLoaded() &&
                       !command.Desc.StartTime;
      voice->Position = voice->Pcm ? 0 : command.Desc.StartOffset;
      voice->Volume.Set(command.Desc.Volume, Now);
      voice->Pitch.Set(command.Desc.Pitch, Now);
      voice->HF.Set(command.Desc.GainHF, Now);
      voice->Fade.From = voice->Fade.To = command.Desc.FadeIn > 0 ? 0.0f : 1.0f;
      voice->Fade.Set(1, voice->Desc.StartTime, command.Desc.FadeIn);
      const auto &d = command.Desc;
      const float spatial[]{d.Position.x, d.Position.y, d.Position.z,
                            d.Velocity.x, d.Velocity.y, d.Velocity.z,
                            d.Radius};
      for (size_t i = 0; i < voice->Spatial.size(); ++i)
        voice->Spatial[i].Set(spatial[i], Now);
      if (voice->Pcm) {
        voice->Played = voice->Pcm->ReadCount();
        voice->LastTime = Now;
      }
      voice->Paused = Buses[d.Bus].Paused;
      Advance(*voice);
      if (!voice->Ended) Voices.emplace(id, std::move(voice));
      return;
    }
    if (command.Op == AudioOp::Listener) {
      const auto &l = command.Listener;
      alListener3f(AL_POSITION, l.Position.x, l.Position.y, l.Position.z);
      alListener3f(AL_VELOCITY, l.Velocity.x, l.Velocity.y, l.Velocity.z);
      const float orientation[]{l.Forward.x, l.Forward.y, l.Forward.z,
                                l.Up.x,      l.Up.y,      l.Up.z};
      alListenerfv(AL_ORIENTATION, orientation);
      return;
    }
    if (command.Op == AudioOp::Speed) {
      alSpeedOfSound(command.Value);
      return;
    }
    if (command.Op == AudioOp::Doppler) {
      alDopplerFactor(command.Value);
      return;
    }
    if (command.Op >= AudioOp::BusVolume && command.Op <= AudioOp::BusGainHF) {
      if (id >= Buses.size() || !Control->Buses[id].load()) return;
      auto &bus = Buses[id];
      if (command.Op == AudioOp::BusVolume)
        bus.Volume.Set(command.Value, Now, command.Ramp);
      if (command.Op == AudioOp::BusGainHF)
        bus.GainHF.Set(command.Value, Now, command.Ramp);
      if (command.Op == AudioOp::BusMute) bus.Mute = command.Value != 0;
      if (command.Op == AudioOp::BusPaused) bus.Pause = command.Value != 0;
      return;
    }
    const auto found = Voices.find(id);
    if (found == Voices.end() || found->second->Ended) return;
    auto &voice = *found->second;
    switch (command.Op) {
      case AudioOp::Pause:
        voice.Pausing = true;
        voice.Stopping = false;
        voice.Fade.Set(0, Now, command.Ramp);
        break;
      case AudioOp::Resume:
        voice.OwnPause = voice.Pausing = voice.Stopping = false;
        voice.Fade.Set(1, Now, command.Ramp);
        break;
      case AudioOp::Bus:
        voice.Desc.Bus = command.Desc.Bus;
        break;
      case AudioOp::Stop:
        voice.Stopping = true;
        voice.Fade.Set(0, Now, command.Ramp);
        if (command.Ramp == 0 || voice.Paused) Finish(voice, true);
        break;
      case AudioOp::Volume:
        voice.Volume.Set(command.Value, Now, command.Ramp);
        break;
      case AudioOp::Pitch:
        voice.Pitch.Set(command.Value, Now, command.Ramp);
        break;
      case AudioOp::Audibility:
        voice.Desc.Audibility = command.Value;
        break;
      case AudioOp::GainHF:
        voice.HF.Set(command.Value, Now, command.Ramp);
        break;
      case AudioOp::Spatial: {
        const auto &d = command.Desc;
        const float values[]{d.Position.x, d.Position.y, d.Position.z,
                             d.Velocity.x, d.Velocity.y, d.Velocity.z,
                             d.Radius};
        for (size_t i = 0; i < voice.Spatial.size(); ++i)
          voice.Spatial[i].Set(values[i], Now, command.Ramp);
        break;
      }
      default:
        break;
    }
  }
  float Score(const Voice &voice) const {
    float level = voice.Desc.Audibility;
    if (level == 0 && voice.Samples) {
      const auto &segment = voice.Samples->Data->Segments[voice.Desc.Segment];
      const auto index = std::min(static_cast<size_t>(voice.Position / 0.02),
                                  segment.Rms.size() - 1);
      level = segment.Rms[index];
    } else if (level == 0)
      level = 1;
    return level * voice.Volume.At(Now) * voice.Fade.At(Now) *
           Buses[voice.Desc.Bus].Gain;
  }
  void Configure(Voice &voice) {
    const auto &source = Sources[voice.Source];
    const auto &bus = Buses[voice.Desc.Bus];
    const auto gain = voice.Volume.At(Now) * voice.Fade.At(Now) * bus.Gain;
    alSourcef(source.Id, AL_MAX_GAIN, std::max(1.0f, gain));
    alSourcef(source.Id, AL_GAIN, std::max(1.0f, gain));
    alSourcef(source.Id, AL_PITCH, voice.Pitch.At(Now));
    Filterf(source.Filter, AL_LOWPASS_GAIN, std::min(1.0f, gain));
    Filterf(source.Filter, AL_LOWPASS_GAINHF, voice.HF.At(Now) * bus.HF);
    alSourcei(source.Id, AL_DIRECT_FILTER, static_cast<ALint>(source.Filter));
    alSourcei(source.Id, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source.Id, AL_SOURCE_SPATIALIZE_SOFT,
              voice.Desc.Spatial ? AL_TRUE : AL_FALSE);
    alSourcei(source.Id, AL_DIRECT_CHANNELS_SOFT,
              voice.Desc.Spatial ? AL_FALSE : AL_TRUE);
    alSource3f(source.Id, AL_POSITION, voice.Spatial[0].At(Now),
               voice.Spatial[1].At(Now), voice.Spatial[2].At(Now));
    alSource3f(source.Id, AL_VELOCITY, voice.Spatial[3].At(Now),
               voice.Spatial[4].At(Now), voice.Spatial[5].At(Now));
    alSourcef(source.Id, AL_SOURCE_RADIUS, voice.Spatial[6].At(Now));
  }
  void Acquire(Voice &voice) {
    voice.Source = FreeSources.back();
    FreeSources.pop_back();
    const auto &source = Sources[voice.Source];
    alSourceRewind(source.Id);
    alSourcei(source.Id, AL_LOOPING, AL_FALSE);
    if (voice.Pcm) {
      voice.Feed = std::make_unique<PcmFeed>();
      voice.Feed->Stream = voice.Pcm.Get();
      voice.Feed->Base = voice.Pcm->ReadCount();
      BufferCallback(source.Callback,
                     voice.Pcm->GetChannels() == 1 ? AL_FORMAT_MONO_FLOAT32
                                                   : AL_FORMAT_STEREO_FLOAT32,
                     static_cast<ALsizei>(voice.Pcm->GetSampleRate()),
                     PcmFeed::Read, voice.Feed.get());
      alSourcei(source.Id, AL_BUFFER, static_cast<ALint>(source.Callback));
    } else {
      const auto &data = *voice.Samples->Data.Get();
      if (voice.Desc.Spatial && data.Channels != 1) {
        Finish(voice, false);
        return;
      }
      const auto &segment = data.Segments[voice.Desc.Segment];
      const auto buffer =
          Samples.at(voice.Samples->Data.Get()).Buffers[voice.Desc.Segment];
      alSourcei(source.Id, AL_BUFFER, static_cast<ALint>(buffer));
      alSourcei(source.Id, AL_LOOPING, segment.Loop ? AL_TRUE : AL_FALSE);
      alSourcei(source.Id, AL_SAMPLE_OFFSET,
                static_cast<ALint>(std::llround(voice.Position * data.Rate)));
    }
    Configure(voice);
    const auto start = static_cast<ALint64SOFT>(
        std::llround(std::max(Now, voice.Desc.StartTime) * 1e9));
    PlayAt(1, &source.Id, start);
  }
  void Select() {
    std::vector<Voice *> candidates;
    size_t pausedSources = 0;
    for (auto &[id, item] : Voices) {
      auto &voice = *item;
      if (voice.Ended) continue;
      if (voice.Pausing && Now >= voice.Fade.End) {
        voice.OwnPause = true;
        voice.Pausing = false;
      }
      const bool paused = voice.OwnPause || Buses[voice.Desc.Bus].Paused;
      if (voice.Source >= 0 && paused != voice.Paused) {
        if (paused)
          alSourcePause(Sources[voice.Source].Id);
        else {
          const auto start = static_cast<ALint64SOFT>(
              std::llround(std::max(Now, voice.Desc.StartTime) * 1e9));
          PlayAt(1, &Sources[voice.Source].Id, start);
        }
      }
      voice.Paused = paused;
      if (voice.Samples && !voice.Samples->IsLoaded()) {
        Control->SetStatus(id, MGDF_VOICE_PENDING);
        continue;
      }
      if (paused) {
        if (voice.Source >= 0) ++pausedSources;
        Control->SetStatus(id, MGDF_VOICE_PAUSED);
        continue;
      }
      candidates.push_back(&voice);
    }
    std::sort(candidates.begin(), candidates.end(),
              [&](const Voice *a, const Voice *b) {
                if (a->Desc.Priority != b->Desc.Priority)
                  return a->Desc.Priority > b->Desc.Priority;
                const auto as = Score(*a), bs = Score(*b);
                return as == bs ? a->Id < b->Id : as > bs;
              });
    const auto available = Sources.size() - pausedSources;
    for (size_t i = available; i < candidates.size(); ++i)
      ReleaseSource(*candidates[i]);
    for (size_t i = 0; i < candidates.size(); ++i) {
      auto &voice = *candidates[i];
      if (i < available) {
        if (voice.Source < 0)
          Acquire(voice);
        else
          Configure(voice);
        if (!voice.Ended)
          Control->SetStatus(voice.Id, Now < voice.Desc.StartTime
                                           ? MGDF_VOICE_PENDING
                                           : MGDF_VOICE_PLAYING);
      } else
        Control->SetStatus(voice.Id, Now < voice.Desc.StartTime
                                         ? MGDF_VOICE_PENDING
                                         : MGDF_VOICE_VIRTUAL);
    }
  }
  void Publish(float elapsed) {
    MixerSnapshot snapshot;
    snapshot.Time = Now;
    snapshot.Stats.AudioThreadMs = elapsed;
    snapshot.Stats.FreeSources = static_cast<UINT32>(FreeSources.size());
    snapshot.Stats.Underruns = Underruns;
    snapshot.Stats.DroppedAudioBlocks = Capture->Counts->Dropped.load();
    if (Output) {
      snapshot.Stats.DeviceChanges = Output->DeviceChanges.load();
      snapshot.Stats.OutputThreadMs = Output->RenderMs.load();
    }
    for (const auto &[id, voice] : Voices) {
      if (voice->Ended) continue;
      if (voice->FileStream) ++snapshot.Stats.Streams;
      if (voice->Source >= 0)
        ++snapshot.Stats.RealVoices;
      else if (Control->GetStatus(id) == MGDF_VOICE_VIRTUAL)
        ++snapshot.Stats.VirtualVoices;
      if (voice->Feed)
        snapshot.Stats.Underruns += voice->Feed->Underruns.load();
    }
    Control->Snapshot.Publish(snapshot);
  }
  void Update() {
    const auto begin = std::chrono::steady_clock::now();
    Now = Clock();
    UploadSamples();
    for (auto &[id, voice] : Voices)
      if (!voice->Ended) Advance(*voice);
    RefreshBuses();
    auto *batch = Control->Submissions.Take();
    while (batch) {
      auto *next = batch->Next;
      for (auto &command : batch->Commands) Apply(command);
      delete batch;
      batch = next;
    }
    RefreshBuses();
    Select();
    std::erase_if(Voices,
                  [](const auto &entry) { return entry.second->Ended; });
    Publish(std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - begin)
                .count());
  }
  void Close() {
    if (Output) Output->Stop();
    Discard(Control->Submissions.Close(), *Control.Get());
    auto *registration = Registrations.Close();
    while (registration) {
      std::unique_ptr<SampleRegistration> current(registration);
      registration = registration->Next;
    }
    auto *retired = Control->RetiredSamples.Close();
    while (retired) {
      std::unique_ptr<SampleRetirement> current(retired);
      retired = retired->Next;
    }
    if (Context) {
      for (auto &[id, voice] : Voices)
        if (!voice->Ended) Finish(*voice, true);
      Voices.clear();
      for (auto &[key, record] : Samples)
        if (!record.Buffers.empty())
          alDeleteBuffers(static_cast<ALsizei>(record.Buffers.size()),
                          record.Buffers.data());
      Samples.clear();
      for (auto &source : Sources) {
        alDeleteSources(1, &source.Id);
        alDeleteBuffers(1, &source.Callback);
        if (DeleteFilters) DeleteFilters(1, &source.Filter);
      }
      Sources.clear();
      FreeSources.clear();
      const auto bind = std::bit_cast<PFNALCSETTHREADCONTEXTPROC>(
          alcGetProcAddress(Device, "alcSetThreadContext"));
      if (bind) bind(nullptr);
      alcDestroyContext(Context);
      Context = nullptr;
    }
    if (Device) {
      alcCloseDevice(Device);
      Device = nullptr;
    }
  }
};

OpenALSoundManagerComponentImpl::OpenALSoundManagerComponentImpl(
    AudioOptions options)
    : _impl(std::make_unique<Impl>(options)) {}
OpenALSoundManagerComponentImpl::~OpenALSoundManagerComponentImpl() { Stop(); }
HRESULT OpenALSoundManagerComponentImpl::Init() {
  auto &m = *_impl;
  if (!m.Options.Loopback) m.Options.SampleRate = MixRate;
  if (!m.Options.Sources || m.Options.Sources > 512) return E_INVALIDARG;
  HRESULT result = E_FAIL;
  if (m.Options.Loopback)
    result = m.Open();
  else {
    std::promise<HRESULT> ready;
    auto future = ready.get_future();
    m.AudioThread = std::thread([&m, ready = std::move(ready)]() mutable {
      HRESULT opened = E_FAIL;
      try {
        opened = m.Open();
        if (SUCCEEDED(opened)) {
          m.Output = std::make_unique<AudioOutput>(
              m.Device, m.Options.Hrtf, m.Capture, m.Options.OutputFile,
              m.Options.EndpointSource);
          opened = m.Output->Start();
        }
      } catch (...) {
      }
      ready.set_value(opened);
      if (SUCCEEDED(opened)) {
        try {
          while (!m.Stopping.load()) {
            const auto next =
                std::chrono::steady_clock::now() + std::chrono::milliseconds(5);
            m.Update();
            std::this_thread::sleep_until(next);
          }
        } catch (...) {
          m.Stopping.store(true);
        }
      }
      m.Close();
    });
    result = future.get();
  }
  if (FAILED(result)) {
    Stop();
    return result;
  }
  m.Streams = std::make_unique<StreamWorker>(m.Control);
  m.Decoder = std::thread([&m] {
    while (!m.Stopping.load()) {
      auto *job = m.Decodes.Take();
      if (!job) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      while (job) {
        auto *next = job->Next;
        DecodeSamples(*job);
        delete job;
        job = next;
        {
          const std::lock_guard lock(m.DecodeMutex);
          m.Outstanding.fetch_sub(1);
        }
        m.DecodeComplete.notify_all();
      }
    }
  });
  return S_OK;
}
ComObject<AudioCaptureSource> OpenALSoundManagerComponentImpl::CaptureSource() {
  return _impl->Capture;
}
void OpenALSoundManagerComponentImpl::Stop() {
  auto &m = *_impl;
  m.Stopping.store(true);
  if (m.Streams) m.Streams->Stop();
  if (m.Decoder.joinable()) m.Decoder.join();
  auto *job = m.Decodes.Close();
  while (job) {
    auto *next = job->Next;
    job->Samples->Data->Status.store(SampleData::Failed);
    delete job;
    job = next;
  }
  if (m.AudioThread.joinable())
    m.AudioThread.join();
  else if (m.Options.Loopback)
    m.Close();
}
MGDFBusId OpenALSoundManagerComponentImpl::GetBus(MGDFBusKind kind) {
  return kind >= MGDF_BUS_MASTER && kind <= MGDF_BUS_MUSIC
             ? static_cast<MGDFBusId>(kind)
             : 0;
}
HRESULT OpenALSoundManagerComponentImpl::CreateBus(const MGDFBusDesc *desc,
                                                   MGDFBusId *bus) try {
  if (!bus) return E_POINTER;
  *bus = 0;
  auto &m = *_impl;
  if (m.Stopping.load()) return E_ABORT;
  if (!desc || !desc->Name || desc->Parent >= AudioControl::BusSlots ||
      !m.Control->Buses[desc->Parent].load() || !std::isfinite(desc->Volume) ||
      desc->Volume < 0)
    return E_INVALIDARG;
  auto info = std::make_unique<AudioBusInfo>();
  info->Name = desc->Name;
  info->Parent = desc->Parent;
  info->InitialVolume = desc->Volume;
  info->Volume.store(desc->Volume);
  if (desc->PreferenceKey) info->Preference = desc->PreferenceKey;
  const auto id = m.Control->NextBus.fetch_add(1);
  if (id >= AudioControl::BusSlots) return E_OUTOFMEMORY;
  m.Control->Buses[id].store(info.release());
  *bus = id;
  return S_OK;
} catch (const std::bad_alloc &) {
  return E_OUTOFMEMORY;
}
HRESULT OpenALSoundManagerComponentImpl::CreateSoundSamples(
    IMGDFReadOnlyFile *file, const MGDFSoundSamplesDesc *desc,
    IMGDFSoundSamples **samples) try {
  if (!samples) return E_POINTER;
  *samples = nullptr;
  auto &m = *_impl;
  if (m.Stopping.load()) return E_ABORT;
  if (!file || (desc && desc->SegmentCount && !desc->Segments))
    return E_INVALIDARG;
  auto data = MakeCom<SampleData>();
  if (desc && desc->SegmentCount)
    data->Descriptors.assign(desc->Segments,
                             desc->Segments + desc->SegmentCount);
  else
    data->Descriptors.push_back({0, 0, FALSE});
  for (const auto &segment : data->Descriptors)
    if (!std::isfinite(segment.Start) || !std::isfinite(segment.Length) ||
        segment.Start < 0 || segment.Length < 0)
      return E_INVALIDARG;
  auto result = MakeCom<SoundSamples>(m.Control, data);
  auto registration = std::make_unique<SampleRegistration>();
  registration->Data = data;
  if (!m.Registrations.Push(registration.get())) return E_ABORT;
  registration.release();
  auto job = std::make_unique<DecodeJob>();
  job->Samples = result;
  job->File = MakeComFromPtr<IMGDFReadOnlyFile>(file);
  m.Outstanding.fetch_add(1);
  if (!m.Decodes.Push(job.get())) {
    m.Outstanding.fetch_sub(1);
    data->Status.store(SampleData::Failed);
    return E_ABORT;
  }
  job.release();
  result.AddRawRef(samples);
  return S_OK;
} catch (const std::bad_alloc &) {
  return E_OUTOFMEMORY;
}
HRESULT OpenALSoundManagerComponentImpl::CreatePcmStream(
    const MGDFSoundPcmDesc *desc, IMGDFSoundPcmStream **stream) try {
  if (!stream) return E_POINTER;
  *stream = nullptr;
  auto &m = *_impl;
  if (m.Stopping.load()) return E_ABORT;
  if (!desc || desc->Channels < 1 || desc->Channels > 2) return E_INVALIDARG;
  auto value = *desc;
  if (!value.SampleRate) value.SampleRate = GetSampleRate();
  if (!value.CapacityFrames) value.CapacityFrames = value.SampleRate / 5;
  if (value.SampleRate < 8000 || value.SampleRate > 192000 ||
      !value.CapacityFrames || value.CapacityFrames > 16777216)
    return E_INVALIDARG;
  auto result = MakeCom<SoundPcmStream>(m.Control, value);
  result.AddRawRef(stream);
  return S_OK;
} catch (const std::bad_alloc &) {
  return E_OUTOFMEMORY;
}
HRESULT OpenALSoundManagerComponentImpl::CreateSoundStream(
    IMGDFReadOnlyFile *file, IMGDFSoundStream **stream) {
  if (!stream) return E_POINTER;
  *stream = nullptr;
  if (_impl->Stopping.load()) return E_ABORT;
  if (!file) return E_INVALIDARG;
  return _impl->Streams->Create(file, stream);
}
bool OpenALSoundManagerComponentImpl::WaitForStreams(
    std::chrono::milliseconds timeout) {
  return _impl->Streams->WaitForUpdate(timeout);
}
HRESULT OpenALSoundManagerComponentImpl::CreateCommandList(
    IMGDFSoundCommandList **list) try {
  if (!list) return E_POINTER;
  *list = nullptr;
  if (_impl->Stopping.load()) return E_ABORT;
  auto result = MakeCom<SoundCommandList>(_impl->Control);
  result.AddRawRef(list);
  return S_OK;
} catch (const std::bad_alloc &) {
  return E_OUTOFMEMORY;
}
void OpenALSoundManagerComponentImpl::Submit(IMGDFSoundCommandList *list) {
  auto *commands = dynamic_cast<SoundCommandList *>(list);
  if (!commands || commands->Owner != _impl->Control) return;
  auto *batch = commands->Detach();
  if (batch && !_impl->Control->Submissions.Push(batch))
    Discard(batch, *_impl->Control.Get());
}
double OpenALSoundManagerComponentImpl::GetAudioTime() {
  return _impl->Control->Snapshot.Read().Time;
}
UINT32 OpenALSoundManagerComponentImpl::GetSampleRate() {
  return _impl->Control->SampleRate.load();
}
MGDFVoiceStatus OpenALSoundManagerComponentImpl::GetVoiceStatus(
    MGDFVoiceId id) {
  return _impl->Control->GetStatus(id);
}
void OpenALSoundManagerComponentImpl::GetStats(MGDFSoundStats *stats) {
  if (stats) *stats = _impl->Control->Snapshot.Read().Stats;
}
void OpenALSoundManagerComponentImpl::SetSpeedOfSound(float value) {
  if (!std::isfinite(value) || value <= 0) return;
  auto list = MakeCom<SoundCommandList>(_impl->Control);
  AudioCommand command;
  command.Op = AudioOp::Speed;
  command.Value = value;
  list->Add(std::move(command));
  Submit(list);
}
void OpenALSoundManagerComponentImpl::SetDopplerFactor(float value) {
  if (!std::isfinite(value) || value < 0) return;
  auto list = MakeCom<SoundCommandList>(_impl->Control);
  AudioCommand command;
  command.Op = AudioOp::Doppler;
  command.Value = value;
  list->Add(std::move(command));
  Submit(list);
}
void OpenALSoundManagerComponentImpl::GetPreferences(
    IMGDFPreferenceSet **preferences) {
  if (!preferences) return;
  auto result = MakeCom<PreferenceSetImpl>();
  for (auto &slot : _impl->Control->Buses) {
    const auto *bus = slot.load();
    if (bus && !bus->Preference.empty())
      result->Preferences[bus->Preference] = std::to_string(bus->Volume.load());
  }
  result->Preferences["host.hrtf"] = _impl->Options.Hrtf ? "1" : "0";
  result.AddRawRef(preferences);
}
void OpenALSoundManagerComponentImpl::Update() {
  _ASSERTE(_impl->Options.Loopback);
  _impl->Update();
}
void OpenALSoundManagerComponentImpl::Render(float *stereo, UINT32 frames) {
  _ASSERTE(_impl->Options.Loopback);
  _impl->RenderSamples(_impl->Device, stereo, static_cast<ALCsizei>(frames));
  _impl->Update();
}
void OpenALSoundManagerComponentImpl::RenderAt(float *stereo, UINT32 frames,
                                               int64_t qpcTime) {
  _ASSERTE(_impl->Options.Loopback);
  UINT32 offset = 0;
  while (offset < frames) {
    const auto count = std::min(frames - offset, MixBlockFrames);
    const auto first =
        static_cast<UINT64>(std::llround(_impl->Clock() * GetSampleRate()));
    Render(stereo + size_t(offset) * 2, count);
    _impl->Capture->Publish(
        stereo + size_t(offset) * 2, 2, count, first,
        qpcTime + int64_t(offset) * AudioTicksPerSecond / GetSampleRate());
    offset += count;
  }
}
bool OpenALSoundManagerComponentImpl::WaitForLoads(
    std::chrono::milliseconds timeout) {
  std::unique_lock lock(_impl->DecodeMutex);
  return _impl->DecodeComplete.wait_for(
      lock, timeout, [&] { return _impl->Outstanding.load() == 0; });
}
}  // namespace MGDF::core::audio::openal_audio
