#include "Stdafx.h"
#undef min
#undef max

#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

#include "SoundStream.hpp"
#include "vendor/dr_wav.h"

namespace MGDF::core::audio::openal_audio {
namespace {
class StreamFile {
 public:
  ~StreamFile() {
    if (_oggOpen) ov_clear(&_ogg);
    if (_wavOpen) drwav_uninit(&_wav);
  }
  bool Open(IMGDFReadOnlyFile *file) {
    if (FAILED(file->Open(_reader.Assign()))) return false;
    char magic[4]{};
    if (_reader->Read(magic, 4) != 4) return false;
    _reader->SetPosition(0);
    if (memcmp(magic, "OggS", 4) == 0) {
      const ov_callbacks callbacks{ReadOgg, Seek, nullptr, TellOgg};
      if (ov_open_callbacks(this, &_ogg, nullptr, 0, callbacks) != 0)
        return false;
      _oggOpen = true;
      const auto *info = ov_info(&_ogg, -1);
      const auto frames = ov_pcm_total(&_ogg, -1);
      if (!info || info->channels < 1 || info->channels > 2 || frames <= 0 ||
          info->rate < 8000 || info->rate > 192000)
        return false;
      Channels = static_cast<UINT32>(info->channels);
      Rate = static_cast<UINT32>(info->rate);
      Frames = static_cast<UINT64>(frames);
      for (long i = 0; i < ov_streams(&_ogg); ++i) {
        const auto *section = ov_info(&_ogg, static_cast<int>(i));
        if (!section || section->channels != info->channels ||
            section->rate != info->rate)
          return false;
      }
      const auto *comments = ov_comment(&_ogg, 0);
      UINT64 start = 0, length = 0;
      bool hasStart = false, hasLength = false;
      for (int i = 0; comments && i < comments->comments; ++i) {
        const std::string_view value(comments->user_comments[i],
                                     comments->comment_lengths[i]);
        const auto read = [&](std::string_view key, UINT64 &number) {
          if (value.size() <= key.size() ||
              _strnicmp(value.data(), key.data(), key.size()) != 0)
            return false;
          const auto result = std::from_chars(
              value.data() + key.size(), value.data() + value.size(), number);
          return result.ec == std::errc{} &&
                 result.ptr == value.data() + value.size();
        };
        hasStart |= read("LOOPSTART=", start);
        hasLength |= read("LOOPLENGTH=", length);
      }
      if (hasStart && hasLength && start < Frames && length &&
          length <= Frames - start) {
        LoopStart = start;
        LoopEnd = start + length;
      }
      return SeekFrame(0);
    }
    if (!drwav_init(&_wav, ReadWav, SeekWav, TellWav, this, nullptr))
      return false;
    _wavOpen = true;
    Channels = _wav.channels;
    Rate = _wav.sampleRate;
    Frames = _wav.totalPCMFrameCount;
    return Channels >= 1 && Channels <= 2 && Rate >= 8000 && Rate <= 192000 &&
           Frames > 0;
  }
  bool SeekFrame(UINT64 frame) {
    return _oggOpen ? ov_pcm_seek(&_ogg, static_cast<ogg_int64_t>(frame)) == 0
                    : drwav_seek_to_pcm_frame(&_wav, frame) != 0;
  }
  bool Read(float *output, UINT32 frames) {
    if (_oggOpen) {
      UINT32 offset = 0;
      while (offset < frames) {
        float **channels = nullptr;
        int section = 0;
        const auto count = ov_read_float(
            &_ogg, &channels, static_cast<int>(frames - offset), &section);
        if (count <= 0) return false;
        for (long i = 0; i < count; ++i)
          for (UINT32 c = 0; c < Channels; ++c)
            output[(offset + i) * Channels + c] = channels[c][i];
        offset += static_cast<UINT32>(count);
      }
    } else if (drwav_read_pcm_frames_f32(&_wav, frames, output) != frames)
      return false;
    return std::all_of(output, output + static_cast<size_t>(frames) * Channels,
                       [](float value) { return std::isfinite(value); });
  }
  UINT32 Channels = 0, Rate = 0;
  UINT64 Frames = 0, LoopStart = 0, LoopEnd = 0;

 private:
  static size_t ReadWav(void *user, void *output, size_t bytes) {
    const auto &file = *static_cast<StreamFile *>(user);
    const auto position = file._reader->GetPosition();
    const auto size = file._reader->GetSize();
    if (position < 0 || position >= size) return 0;
    const auto count =
        std::min<UINT64>(bytes, static_cast<UINT64>(size - position));
    return file._reader->Read(
        output, static_cast<UINT32>(std::min<UINT64>(count, UINT32_MAX)));
  }
  static size_t ReadOgg(void *output, size_t size, size_t count, void *user) {
    if (!size || size > UINT32_MAX) return 0;
    count = std::min(count, UINT32_MAX / size);
    return ReadWav(user, output, size * count) / size;
  }
  static int Seek(void *user, ogg_int64_t offset, int origin) {
    const auto &reader = static_cast<StreamFile *>(user)->_reader;
    const auto base = origin == SEEK_SET   ? 0
                      : origin == SEEK_CUR ? reader->GetPosition()
                                           : reader->GetSize();
    if (base < 0 || offset < -base || offset > reader->GetSize() - base)
      return -1;
    reader->SetPosition(base + offset);
    return reader->GetPosition() == base + offset ? 0 : -1;
  }
  static long TellOgg(void *user) {
    const auto pos = static_cast<StreamFile *>(user)->_reader->GetPosition();
    return pos >= 0 && pos <= LONG_MAX ? static_cast<long>(pos) : -1;
  }
  static drwav_bool32 SeekWav(void *user, int offset,
                              drwav_seek_origin origin) {
    return Seek(user, offset,
                origin == DRWAV_SEEK_SET   ? SEEK_SET
                : origin == DRWAV_SEEK_CUR ? SEEK_CUR
                                           : SEEK_END) == 0;
  }
  static drwav_bool32 TellWav(void *user, drwav_int64 *position) {
    *position = static_cast<StreamFile *>(user)->_reader->GetPosition();
    return *position >= 0;
  }
  ComObject<IMGDFFileReader> _reader;
  OggVorbis_File _ogg{};
  drwav _wav{};
  bool _oggOpen = false, _wavOpen = false;
};

struct StreamSignal {
  std::mutex Mutex;
  std::condition_variable Wake, Complete;
  std::atomic<bool> Stopped{false};
  UINT64 Requested = 0, Completed = 0;
  void Notify() {
    {
      const std::lock_guard lock(Mutex);
      ++Requested;
    }
    Wake.notify_one();
  }
};
enum class StreamOp { Play, Pause, Stop, Volume, Bus, Loop };
struct StreamCommand {
  StreamCommand *Next = nullptr;
  StreamOp Op = StreamOp::Stop;
  double Value = 0, End = 0;
  float Ramp = 0;
};
struct StreamSnapshot {
  double Position = 0;
  bool Playing = false;
};
struct PositionMap {
  UINT64 Begin = 0, FileBegin = 0, LoopStart = 0, LoopEnd = 0;
  UINT64 At(UINT64 frame) const {
    const auto position = FileBegin + frame - Begin;
    return LoopEnd && position >= LoopEnd
               ? LoopStart + (position - LoopEnd) % (LoopEnd - LoopStart)
               : position;
  }
};
struct StreamState {
  std::shared_ptr<AudioControl> Owner;
  std::unique_ptr<StreamFile> File;
  std::wstring Name;
  AudioQueue<StreamCommand> Commands;
  AudioSnapshot<StreamSnapshot> Snapshot;
  std::atomic<bool> Released{false};
  ComObject<SoundPcmStream> Pcm;
  std::deque<PositionMap> Mapping;
  UINT64 Cursor = 0, Written = 0, LoopStart = 0, LoopEnd = 0;
  MGDFVoiceId Voice = 0;
  MGDFBusId Bus = MGDF_BUS_MUSIC;
  float Volume = 1;
  bool WantsPlay = false, Stopping = false;
  std::array<float, 8192> Buffer{};

  void Submit(const ComObject<SoundCommandList> &list) {
    auto *batch = list->Detach();
    if (batch && !Owner->Submissions.Push(batch)) {
      for (auto &command : batch->Commands) {
        if (command.Op == AudioOp::PlayPcm) {
          Owner->SetStatus(command.Voice, MGDF_VOICE_ENDED);
          command.Pcm->ReleaseVoice(true);
        }
      }
      delete batch;
    }
  }
  void Control(AudioOp op, float ramp = 0) {
    auto list = MakeCom<SoundCommandList>(Owner);
    AudioCommand command;
    command.Op = op;
    command.Voice = Voice;
    command.Ramp = ramp;
    command.Desc.Bus = Bus;
    list->Add(std::move(command));
    Submit(list);
  }
  bool Fill() {
    auto writable = Pcm->GetWritableFrames();
    while (writable) {
      const auto end = LoopEnd ? LoopEnd : File->Frames;
      if (Cursor >= end) {
        if (!LoopEnd) {
          Pcm->End();
          break;
        }
        Cursor = LoopStart;
        if (!File->SeekFrame(Cursor)) return false;
      }
      const auto count = static_cast<UINT32>(
          std::min<UINT64>(std::min(writable, 4096U), end - Cursor));
      if (!File->Read(Buffer.data(), count)) return false;
      const auto written = Pcm->Write(Buffer.data(), count);
      Cursor += written;
      Written += written;
      writable -= written;
      if (written != count) break;
    }
    if (!LoopEnd && Cursor == File->Frames) Pcm->End();
    return true;
  }
  void Start(float fade) {
    Pcm = MakeCom<SoundPcmStream>(
        Owner, MGDFSoundPcmDesc{File->Channels, File->Rate, File->Rate * 2});
    Cursor = Written = 0;
    Mapping.clear();
    Mapping.push_back({0, 0, LoopStart, LoopEnd});
    if (!File->SeekFrame(0) || !Fill()) {
      Pcm.Clear();
      return;
    }
    Voice = Owner->NewVoice();
    if (!Voice) {
      Pcm.Clear();
      return;
    }
    Pcm->Claim(Voice);
    auto list = MakeCom<SoundCommandList>(Owner);
    AudioCommand command;
    command.Op = AudioOp::PlayPcm;
    command.Voice = Voice;
    command.Pcm = Pcm;
    command.FileStream = true;
    command.Desc.Bus = Bus;
    command.Desc.Volume = Volume;
    command.Desc.Pitch = command.Desc.GainHF = command.Desc.Audibility = 1;
    command.Desc.FadeIn = fade;
    if (!list->Add(std::move(command))) {
      Owner->SetStatus(Voice, MGDF_VOICE_ENDED);
      Pcm->ReleaseVoice(true);
    }
    Submit(list);
  }
  void Apply(const StreamCommand &command) {
    const auto ended = Owner->GetStatus(Voice) == MGDF_VOICE_ENDED;
    switch (command.Op) {
      case StreamOp::Play:
        if (ended || Stopping) {
          if (!ended) Control(AudioOp::Stop);
          Start(command.Ramp);
        } else if (!WantsPlay)
          Control(AudioOp::Resume, command.Ramp);
        WantsPlay = true;
        Stopping = false;
        break;
      case StreamOp::Pause:
        if (!ended) Control(AudioOp::Pause, command.Ramp);
        WantsPlay = Stopping = false;
        break;
      case StreamOp::Stop:
        if (!ended) Control(AudioOp::Stop, command.Ramp);
        WantsPlay = false;
        Stopping = true;
        break;
      case StreamOp::Volume: {
        Volume = static_cast<float>(command.Value);
        auto list = MakeCom<SoundCommandList>(Owner);
        list->SetVolume(Voice, Volume, command.Ramp);
        Submit(list);
        break;
      }
      case StreamOp::Bus:
        Bus = static_cast<MGDFBusId>(
            static_cast<UINT64>(std::llround(command.Value)));
        Control(AudioOp::Bus);
        break;
      case StreamOp::Loop:
        LoopStart =
            static_cast<UINT64>(std::llround(command.Value * File->Rate));
        LoopEnd =
            command.End == 0
                ? File->Frames
                : static_cast<UINT64>(std::llround(command.End * File->Rate));
        if (Pcm && !Pcm->Ended.load()) {
          if (Cursor >= LoopEnd) {
            Cursor = LoopStart;
            if (!File->SeekFrame(Cursor)) Control(AudioOp::Stop);
          }
          Mapping.push_back({Written, Cursor, LoopStart, LoopEnd});
        }
        break;
    }
  }
  void Update() {
    auto *command = Commands.Take();
    while (command) {
      std::unique_ptr<StreamCommand> current(command);
      command = command->Next;
      Apply(*current);
    }
    const auto status = Owner->GetStatus(Voice);
    if (Pcm && status != MGDF_VOICE_ENDED && !Fill()) Control(AudioOp::Stop);
    StreamSnapshot snapshot;
    if (Pcm) {
      UINT64 frame = 0;
      Pcm->GetPlayPosition(&frame, nullptr);
      while (Mapping.size() > 1 && Mapping[1].Begin <= frame)
        Mapping.pop_front();
      snapshot.Position =
          static_cast<double>(Mapping.front().At(frame)) / File->Rate;
    }
    if (status == MGDF_VOICE_ENDED && Stopping) {
      snapshot.Position = 0;
      Pcm.Clear();
    }
    snapshot.Playing =
        status == MGDF_VOICE_PLAYING || status == MGDF_VOICE_VIRTUAL;
    Snapshot.Publish(snapshot);
  }
};
class SoundStream : public ComBase<IMGDFSoundStream> {
 public:
  SoundStream(std::shared_ptr<StreamState> state,
              std::shared_ptr<StreamSignal> signal)
      : _state(std::move(state)), _signal(std::move(signal)) {}
  ~SoundStream() {
    _state->Released.store(true);
    _signal->Notify();
  }
  HRESULT __stdcall GetName(wchar_t *name, UINT64 *length) final {
    if (!length) return E_POINTER;
    const auto capacity = *length;
    *length = _state->Name.size() + 1;
    if (!name) return S_OK;
    if (capacity < *length) return E_NOT_SUFFICIENT_BUFFER;
    std::copy_n(_state->Name.c_str(), *length, name);
    return S_OK;
  }
  double __stdcall GetDuration() final {
    return static_cast<double>(_state->File->Frames) / _state->File->Rate;
  }
  double __stdcall GetPosition() final {
    return _state->Snapshot.Read().Position;
  }
  BOOL __stdcall IsPlaying() final { return _state->Snapshot.Read().Playing; }
  void __stdcall SetLoop(double start, double end) final {
    if (!std::isfinite(start) || !std::isfinite(end) || start < 0 || end < 0 ||
        start >= GetDuration() ||
        (end && (end <= start || end > GetDuration())))
      return;
    const auto rate = _state->File->Rate;
    if (std::llround((end ? end : GetDuration()) * rate) <=
        std::llround(start * rate))
      return;
    Send(StreamOp::Loop, start, 0, end);
  }
  void __stdcall Play(float fade) final { Send(StreamOp::Play, 0, fade); }
  void __stdcall Pause(float fade) final { Send(StreamOp::Pause, 0, fade); }
  void __stdcall Stop(float fade) final { Send(StreamOp::Stop, 0, fade); }
  void __stdcall SetVolume(float volume, float ramp) final {
    Send(StreamOp::Volume, volume, ramp);
  }
  void __stdcall SetBus(MGDFBusId bus) final {
    if (bus < AudioControl::BusSlots && _state->Owner->Buses[bus].load())
      Send(StreamOp::Bus, bus);
  }

 private:
  void Send(StreamOp op, double value, float ramp = 0,
            double end = 0) noexcept {
    if (_signal->Stopped.load() || !std::isfinite(value) || value < 0 ||
        !std::isfinite(ramp) || ramp < 0)
      return;
    try {
      auto command = std::make_unique<StreamCommand>();
      command->Op = op;
      command->Value = value;
      command->End = end;
      command->Ramp = ramp;
      if (_state->Commands.Push(command.get())) command.release();
      _signal->Notify();
    } catch (...) {
    }
  }
  const std::shared_ptr<StreamState> _state;
  const std::shared_ptr<StreamSignal> _signal;
};
struct StreamRegistration {
  StreamRegistration *Next = nullptr;
  std::shared_ptr<StreamState> State;
};
}  // namespace

struct StreamWorker::Impl {
  const std::shared_ptr<AudioControl> Control;
  const std::shared_ptr<StreamSignal> Signal = std::make_shared<StreamSignal>();
  AudioQueue<StreamRegistration> Registrations;
  std::vector<std::shared_ptr<StreamState>> Streams;
  std::thread Thread;

  explicit Impl(std::shared_ptr<AudioControl> control)
      : Control(std::move(control)) {
    Thread = std::thread([this] {
      UINT64 completed = 0;
      while (!Signal->Stopped.load()) {
        UINT64 requested;
        {
          std::unique_lock lock(Signal->Mutex);
          Signal->Wake.wait_for(lock, std::chrono::milliseconds(5), [&] {
            return Signal->Stopped.load() || Signal->Requested != completed;
          });
          requested = Signal->Requested;
        }
        auto *registration = Registrations.Take();
        while (registration) {
          std::unique_ptr<StreamRegistration> current(registration);
          registration = registration->Next;
          Streams.push_back(std::move(current->State));
        }
        std::erase_if(Streams, [](auto &stream) {
          if (stream->Released.load()) {
            stream->Control(AudioOp::Stop);
            return true;
          }
          try {
            stream->Update();
          } catch (...) {
            stream->Control(AudioOp::Stop);
          }
          return false;
        });
        {
          const std::lock_guard lock(Signal->Mutex);
          Signal->Completed = completed = requested;
        }
        Signal->Complete.notify_all();
      }
      for (auto &stream : Streams) {
        stream->Control(AudioOp::Stop);
        stream->Snapshot.Publish({});
      }
      Streams.clear();
    });
  }
};

StreamWorker::StreamWorker(std::shared_ptr<AudioControl> control)
    : _impl(std::make_unique<Impl>(std::move(control))) {}
StreamWorker::~StreamWorker() { Stop(); }
void StreamWorker::Stop() {
  _impl->Signal->Stopped.store(true);
  _impl->Signal->Notify();
  if (_impl->Thread.joinable()) _impl->Thread.join();
}
HRESULT StreamWorker::Create(IMGDFReadOnlyFile *file,
                             IMGDFSoundStream **stream) try {
  auto state = std::make_shared<StreamState>();
  state->Owner = _impl->Control;
  state->File = std::make_unique<StreamFile>();
  if (!state->File->Open(file)) return E_FAIL;
  UINT64 length = 0;
  if (FAILED(file->GetLogicalName(nullptr, &length)) || !length) return E_FAIL;
  state->Name.resize(length);
  if (FAILED(file->GetLogicalName(state->Name.data(), &length))) return E_FAIL;
  state->Name.resize(wcslen(state->Name.c_str()));
  state->LoopStart = state->File->LoopStart;
  state->LoopEnd = state->File->LoopEnd;
  auto registration = std::make_unique<StreamRegistration>();
  registration->State = state;
  auto result = MakeCom<SoundStream>(state, _impl->Signal);
  if (_impl->Signal->Stopped.load() ||
      !_impl->Registrations.Push(registration.get()))
    return E_ABORT;
  registration.release();
  _impl->Signal->Notify();
  result.AddRawRef(stream);
  return S_OK;
} catch (const std::bad_alloc &) {
  return E_OUTOFMEMORY;
} catch (...) {
  return E_FAIL;
}
bool StreamWorker::WaitForUpdate(std::chrono::milliseconds timeout) {
  auto &signal = *_impl->Signal;
  std::unique_lock lock(signal.Mutex);
  const auto requested = ++signal.Requested;
  signal.Wake.notify_one();
  return signal.Complete.wait_for(lock, timeout, [&] {
    return signal.Completed >= requested || signal.Stopped.load();
  });
}
}  // namespace MGDF::core::audio::openal_audio
