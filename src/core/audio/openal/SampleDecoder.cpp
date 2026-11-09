#include "Stdafx.h"
#undef min
#undef max

#include <CodeAnalysis/Warnings.h>
#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#include "AudioState.hpp"
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "vendor/dr_wav.h"
#pragma warning(pop)

namespace MGDF::core::audio::openal_audio {
namespace {
constexpr size_t MaxSamples = 128 * 1024 * 1024;
struct MemoryFile {
  const std::vector<unsigned char> &Bytes;
  size_t Position = 0;
  static size_t Read(void *out, size_t size, size_t count, void *user) {
    auto &file = *static_cast<MemoryFile *>(user);
    if (!size) return 0;
    count = std::min(count, (file.Bytes.size() - file.Position) / size);
    memcpy(out, file.Bytes.data() + file.Position, count * size);
    file.Position += count * size;
    return count;
  }
  static int Seek(void *user, ogg_int64_t offset, int origin) {
    auto &file = *static_cast<MemoryFile *>(user);
    const auto base =
        origin == SEEK_SET
            ? 0LL
            : static_cast<long long>(origin == SEEK_CUR ? file.Position
                                                        : file.Bytes.size());
    if (offset < -base ||
        offset > static_cast<ogg_int64_t>(file.Bytes.size()) - base)
      return -1;
    file.Position = static_cast<size_t>(base + offset);
    return 0;
  }
  static long Tell(void *user) {
    return static_cast<long>(static_cast<MemoryFile *>(user)->Position);
  }
};

bool Decode(const std::vector<unsigned char> &bytes, SampleData &data,
            std::vector<float> &pcm) {
  if (bytes.size() >= 4 && memcmp(bytes.data(), "OggS", 4) == 0) {
    MemoryFile memory{bytes};
    OggVorbis_File file{};
    const ov_callbacks callbacks{MemoryFile::Read, MemoryFile::Seek, nullptr,
                                 MemoryFile::Tell};
    if (ov_open_callbacks(&memory, &file, nullptr, 0, callbacks) != 0)
      return false;
    struct Close {
      OggVorbis_File *File;
      ~Close() { ov_clear(File); }
    } close{&file};
    const auto *info = ov_info(&file, -1);
    if (!info || info->channels < 1 || info->channels > 2 || info->rate <= 0 ||
        info->rate > 192000)
      return false;
    data.Channels = static_cast<UINT32>(info->channels);
    data.Rate = static_cast<UINT32>(info->rate);
    for (;;) {
      float **channels = nullptr;
      int section = 0;
      const auto frames = ov_read_float(&file, &channels, 4096, &section);
      if (frames == 0) break;
      if (frames < 0) return false;
      const auto *current = ov_info(&file, section);
      if (!current || current->channels != static_cast<int>(data.Channels) ||
          current->rate != static_cast<long>(data.Rate))
        return false;
      const auto count = static_cast<size_t>(frames) * data.Channels;
      if (pcm.size() + count > MaxSamples) return false;
      const auto base = pcm.size();
      pcm.resize(base + count);
      for (long i = 0; i < frames; ++i)
        for (UINT32 c = 0; c < data.Channels; ++c)
          pcm[base + static_cast<size_t>(i) * data.Channels + c] =
              channels[c][i];
    }
    return !pcm.empty();
  }
  drwav wav{};
  if (!drwav_init_memory(&wav, bytes.data(), bytes.size(), nullptr))
    return false;
  struct Close {
    drwav *File;
    ~Close() { drwav_uninit(File); }
  } close{&wav};
  if (wav.channels < 1 || wav.channels > 2 || !wav.sampleRate ||
      wav.sampleRate > 192000 || !wav.totalPCMFrameCount ||
      wav.totalPCMFrameCount > MaxSamples / wav.channels)
    return false;
  data.Channels = wav.channels;
  data.Rate = wav.sampleRate;
  pcm.resize(static_cast<size_t>(wav.totalPCMFrameCount) * wav.channels);
  return drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, pcm.data()) ==
         wav.totalPCMFrameCount;
}
bool Load(const DecodeJob &job) {
  ComObject<IMGDFFileReader> reader;
  if (FAILED(job.File->Open(reader.Assign()))) return false;
  const auto size = reader->GetSize();
  if (size <= 0 || size > 512 * 1024 * 1024) return false;
  std::vector<unsigned char> bytes(static_cast<size_t>(size));
  size_t read = 0;
  while (read < bytes.size()) {
    const auto count = reader->Read(bytes.data() + read,
                                    static_cast<UINT32>(bytes.size() - read));
    if (!count || count > bytes.size() - read) return false;
    read += count;
  }
  reader.Clear();
  std::vector<float> pcm;
  auto &data = *job.Data;
  if (!Decode(bytes, data, pcm)) return false;
  for (const float value : pcm)
    if (!std::isfinite(value)) return false;
  const auto frames = pcm.size() / data.Channels;
  const auto duration = static_cast<double>(frames) / data.Rate;
  for (const auto &desc : data.Descriptors) {
    if (desc.Start >= duration ||
        (desc.Length > 0 && desc.Start + desc.Length > duration + 1e-9))
      return false;
    const auto first =
        static_cast<size_t>(std::llround(desc.Start * data.Rate));
    const auto length =
        desc.Length > 0
            ? static_cast<size_t>(std::llround(desc.Length * data.Rate))
            : frames - first;
    if (!length || first >= frames || length > frames - first) return false;
    SampleSegment segment;
    segment.Loop = desc.Loop != FALSE;
    segment.Duration = static_cast<double>(length) / data.Rate;
    segment.Samples.assign(pcm.begin() + first * data.Channels,
                           pcm.begin() + (first + length) * data.Channels);
    const auto window = std::max(1U, data.Rate / 50);
    for (size_t offset = 0; offset < length; offset += window) {
      const auto count =
          std::min<size_t>(window, length - offset) * data.Channels;
      double sum = 0;
      for (size_t i = 0; i < count; ++i) {
        const double value = segment.Samples[offset * data.Channels + i];
        sum += value * value;
      }
      segment.Rms.push_back(static_cast<float>(std::sqrt(sum / count)));
    }
    data.Segments.push_back(std::move(segment));
  }
  return true;
}
}  // namespace

void DecodeSamples(const DecodeJob &job) {
  try {
    job.Data->Status.store(Load(job) ? SampleData::Decoded : SampleData::Failed,
                           std::memory_order_release);
  } catch (...) {
    job.Data->Status.store(SampleData::Failed, std::memory_order_release);
  }
}
}  // namespace MGDF::core::audio::openal_audio
