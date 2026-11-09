#include "stdafx.h"
#undef min
#undef max
#include <vorbis/vorbisenc.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <future>
#include <numbers>
#include <thread>

#include "../../src/core/audio/openal/OpenALSoundManagerComponentImpl.hpp"
#include "../../src/core/common/MGDFPreferenceSet.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::audio::openal_audio;
using namespace std::chrono_literals;
namespace {
class AudioTests : public testing::Test {
 protected:
  void SetUp() override { Open(2); }
  void TearDown() override {
    Commands.Clear();
    Manager.Clear();
  }
  void Open(UINT32 sources) {
    Commands.Clear();
    Manager.Clear();
    Manager = MakeCom<OpenALSoundManagerComponentImpl>(AudioOptions{
        .Loopback = true, .SampleRate = 48000, .Sources = sources});
    ASSERT_EQ(S_OK, Manager->Init());
    ASSERT_EQ(S_OK, Manager->CreateCommandList(Commands.Assign()));
  }
  MGDFVoiceDesc Desc() {
    MGDFVoiceDesc desc{};
    desc.Bus = Manager->GetBus(MGDF_BUS_SOUND);
    desc.Volume = 1;
    desc.Pitch = 1;
    desc.GainHF = 1;
    desc.Audibility = 1;
    return desc;
  }
  ComObject<IMGDFSoundPcmStream> Pcm(UINT32 rate = 48000,
                                     UINT32 capacity = 9600,
                                     UINT32 channels = 2) {
    const MGDFSoundPcmDesc desc{channels, rate, capacity};
    ComObject<IMGDFSoundPcmStream> pcm;
    EXPECT_EQ(S_OK, Manager->CreatePcmStream(&desc, pcm.Assign()));
    return pcm;
  }
  std::vector<float> Signal(UINT32 frames) {
    std::vector<float> data(static_cast<size_t>(frames) * 2);
    UINT32 seed = 42;
    for (UINT32 i = 0; i < frames; ++i) {
      seed = 1664525U * seed + 1013904223U;
      const auto noise = static_cast<float>(seed >> 8) / 16777216.0f - 0.5f;
      const auto value =
          0.1f * std::sin(2.0f * std::numbers::pi_v<float> * i / 48.0f) +
          0.05f * noise;
      data[2 * i] = value;
      data[2 * i + 1] = -value;
    }
    return data;
  }
  void Submit() {
    Manager->Submit(Commands);
    Manager->Update();
  }
  std::vector<float> Render(UINT32 frames) {
    std::vector<float> result(static_cast<size_t>(frames) * 2);
    Manager->Render(result.data(), frames);
    return result;
  }
  UINT64 Position(IMGDFSoundPcmStream *pcm) {
    UINT64 frame = 0;
    pcm->GetPlayPosition(&frame, nullptr);
    return frame;
  }
  MGDFSoundStats Stats() {
    MGDFSoundStats value{};
    Manager->GetStats(&value);
    return value;
  }
  void BlockSource() {
    auto pcm = Pcm();
    pcm->WriteSilence(9600);
    auto desc = Desc();
    desc.Priority = 100;
    Commands->PlayPcm(pcm, &desc);
    Submit();
  }
  MGDFBusId Bus(MGDFBusId parent, float volume) {
    const MGDFBusDesc desc{L"Test", parent, volume, nullptr};
    MGDFBusId bus = 0;
    EXPECT_EQ(S_OK, Manager->CreateBus(&desc, &bus));
    return bus;
  }
  ComObject<OpenALSoundManagerComponentImpl> Manager;
  ComObject<IMGDFSoundCommandList> Commands;
};

TEST_F(AudioTests, SubmissionStartsARecordedVoiceAndResetsTheList) {
  auto pcm = Pcm();
  pcm->WriteSilence(1000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  EXPECT_EQ(MGDF_VOICE_PENDING, Manager->GetVoiceStatus(id));
  Render(10);
  EXPECT_EQ(1000U, pcm->GetQueuedFrames());
  Submit();
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  Submit();
  EXPECT_EQ(1U, Stats().RealVoices);
}
TEST_F(AudioTests, ScheduledPcmStartsAtTheRequestedOutputFrame) {
  auto pcm = Pcm();
  std::vector<float> data(2000, 0.125f);
  pcm->Write(data.data(), 1000);
  auto desc = Desc();
  desc.StartTime = 0.01;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  const auto output = Render(960);
  EXPECT_TRUE(std::all_of(output.begin(), output.begin() + 960,
                          [](float x) { return x == 0; }));
  EXPECT_NE(0.0f, output[960]);
}
TEST_F(AudioTests, PcmSignalIncludesVoiceAndAncestorBusGains) {
  const auto parent = Bus(Manager->GetBus(MGDF_BUS_MASTER), 0.5f);
  const auto child = Bus(parent, 0.25f);
  auto pcm = Pcm();
  const auto signal = Signal(1000);
  pcm->Write(signal.data(), 1000);
  auto desc = Desc();
  desc.Bus = child;
  desc.Volume = 0.5f;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  const auto output = Render(480);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[i] * 0.0625f, output[i], 1e-6f) << i;
}
TEST_F(AudioTests, AncestorMuteSilencesItsChild) {
  const auto parent = Bus(0, 1);
  const auto child = Bus(parent, 1);
  auto pcm = Pcm();
  const auto signal = Signal(4000);
  pcm->Write(signal.data(), 4000);
  auto desc = Desc();
  desc.Bus = child;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  Commands->SetBusMute(parent, TRUE);
  Submit();
  Render(480);
  const auto output = Render(480);
  EXPECT_TRUE(std::all_of(output.begin(), output.end(),
                          [](float x) { return x == 0; }));
}
TEST_F(AudioTests, PriorityTakesPrecedenceOverAudibility) {
  Open(1);
  auto a = Pcm(), b = Pcm();
  auto desc = Desc();
  desc.Audibility = 10;
  const auto low = Commands->PlayPcm(a, &desc);
  desc.Priority = 1;
  desc.Audibility = 0.01f;
  const auto high = Commands->PlayPcm(b, &desc);
  Submit();
  EXPECT_EQ(MGDF_VOICE_VIRTUAL, Manager->GetVoiceStatus(low));
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(high));
}
TEST_F(AudioTests, AudibilityChangesWhichVoiceOwnsTheSource) {
  Open(1);
  auto a = Pcm(), b = Pcm(), c = Pcm();
  auto desc = Desc();
  desc.Audibility = 1;
  const auto first = Commands->PlayPcm(a, &desc);
  desc.Audibility = 2;
  const auto second = Commands->PlayPcm(b, &desc);
  desc.Audibility = 3;
  const auto third = Commands->PlayPcm(c, &desc);
  Submit();
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(third));
  Commands->SetAudibility(first, 4);
  Submit();
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(first));
  EXPECT_EQ(MGDF_VOICE_VIRTUAL, Manager->GetVoiceStatus(third));
  Commands->Stop(first, 0);
  Commands->Stop(third, 0);
  Submit();
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(second));
}
TEST_F(AudioTests, EmptyRingProducesSilenceWithoutAdvancingThePosition) {
  auto pcm = Pcm();
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  const auto output = Render(480);
  EXPECT_TRUE(std::all_of(output.begin(), output.end(),
                          [](float x) { return x == 0; }));
  EXPECT_EQ(0U, Position(pcm));
  EXPECT_GT(Stats().Underruns, 0U);
}
TEST_F(AudioTests, PositionAfterAnUnderrunMapsTheNextOutputBlock) {
  auto pcm = Pcm();
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  const auto signal = Signal(2000);
  pcm->Write(signal.data(), 2000);
  Render(480);
  UINT64 frame = 0;
  double time = 0;
  pcm->GetPlayPosition(&frame, &time);
  EXPECT_EQ(456U, frame);
  EXPECT_NEAR(0.02, time, 1e-8);
  const auto predicted =
      frame + static_cast<UINT64>(std::llround((0.03 - time) * 48000));
  Render(480);
  EXPECT_EQ(predicted, Position(pcm));
}
TEST_F(AudioTests, WriteSilenceAdvancesThePlayPosition) {
  auto pcm = Pcm();
  EXPECT_EQ(1000U, pcm->WriteSilence(1000));
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  const auto output = Render(480);
  EXPECT_EQ(480U, Position(pcm));
  EXPECT_EQ(0U, Stats().Underruns);
  EXPECT_TRUE(std::all_of(output.begin(), output.end(),
                          [](float x) { return x == 0; }));
}
TEST_F(AudioTests, QueuedFramesExcludeCallbackReadAhead) {
  auto pcm = Pcm();
  pcm->WriteSilence(1000);
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(1);
  EXPECT_EQ(1U, Position(pcm));
  EXPECT_EQ(975U, pcm->GetQueuedFrames());
}
TEST_F(AudioTests, EndWaitsUntilTheLastReadAheadFrameHasPlayed) {
  auto pcm = Pcm();
  pcm->WriteSilence(16);
  pcm->End();
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(1);
  ASSERT_EQ(0U, pcm->GetQueuedFrames());
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  Render(15);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
  EXPECT_EQ(16U, Position(pcm));
}
TEST_F(AudioTests, VirtualPcmUsesItsStreamRate) {
  Open(1);
  BlockSource();
  auto pcm = Pcm(24000);
  pcm->WriteSilence(2000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  EXPECT_EQ(MGDF_VOICE_VIRTUAL, Manager->GetVoiceStatus(id));
  EXPECT_EQ(240U, Position(pcm));
}
TEST_F(AudioTests, VirtualPcmIncludesPitch) {
  Open(1);
  BlockSource();
  auto pcm = Pcm(24000);
  pcm->WriteSilence(2000);
  auto desc = Desc();
  desc.Pitch = 2;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  EXPECT_EQ(480U, Position(pcm));
}
TEST_F(AudioTests, VirtualPcmKeepsFractionalFrames) {
  Open(1);
  BlockSource();
  auto pcm = Pcm(24000);
  pcm->WriteSilence(2000);
  auto desc = Desc();
  desc.Pitch = 0.5f;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  for (int i = 0; i < 8; ++i) Render(1);
  EXPECT_EQ(2U, Position(pcm));
}
TEST_F(AudioTests, VirtualPcmWaitsForItsScheduledStart) {
  Open(1);
  BlockSource();
  auto pcm = Pcm();
  pcm->WriteSilence(2000);
  auto desc = Desc();
  desc.StartTime = 0.01;
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  EXPECT_EQ(0U, Position(pcm));
  Render(480);
  EXPECT_EQ(480U, Position(pcm));
}
TEST_F(AudioTests, BusPauseStopsVirtualPcmTime) {
  Open(1);
  BlockSource();
  const auto bus = Bus(0, 1);
  auto pcm = Pcm();
  pcm->WriteSilence(2000);
  auto desc = Desc();
  desc.Bus = bus;
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(240);
  Commands->SetBusPaused(bus, TRUE);
  Submit();
  Render(480);
  EXPECT_EQ(MGDF_VOICE_PAUSED, Manager->GetVoiceStatus(id));
  EXPECT_EQ(240U, Position(pcm));
  Commands->SetBusPaused(bus, FALSE);
  Submit();
  Render(240);
  EXPECT_EQ(480U, Position(pcm));
}
TEST_F(AudioTests, VirtualShortfallCountsAnUnderrun) {
  Open(1);
  BlockSource();
  auto pcm = Pcm();
  pcm->WriteSilence(10);
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  const auto before = Stats().Underruns;
  Render(480);
  EXPECT_EQ(10U, Position(pcm));
  EXPECT_GT(Stats().Underruns, before);
}
TEST_F(AudioTests, VirtualTransitionCountsReadAheadAndResumesFromTheRing) {
  Open(1);
  auto a = Pcm(), b = Pcm();
  a->WriteSilence(2000);
  b->WriteSilence(2000);
  auto desc = Desc();
  const auto first = Commands->PlayPcm(a, &desc);
  Submit();
  Render(1);
  desc.Audibility = 2;
  const auto second = Commands->PlayPcm(b, &desc);
  Submit();
  EXPECT_EQ(25U, Position(a));
  Render(480);
  EXPECT_EQ(505U, Position(a));
  Commands->Stop(second, 0);
  Submit();
  Render(1);
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(first));
  EXPECT_EQ(506U, Position(a));
}
TEST_F(AudioTests, StopRejectsWritesAndReplayUsesTheRemainingFrames) {
  auto pcm = Pcm();
  pcm->WriteSilence(2000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(1);
  Commands->Stop(id, 0);
  Submit();
  EXPECT_EQ(0U, pcm->WriteSilence(1));
  const auto next = Commands->PlayPcm(pcm, &desc);
  Submit();
  EXPECT_NE(0U, pcm->WriteSilence(1));
  Render(1);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(next));
  EXPECT_EQ(26U, Position(pcm));
}
TEST_F(AudioTests, OnePcmStreamCannotHaveTwoConsumers) {
  auto pcm = Pcm();
  const auto desc = Desc();
  const auto first = Commands->PlayPcm(pcm, &desc);
  EXPECT_NE(0U, first);
  EXPECT_EQ(0U, Commands->PlayPcm(pcm, &desc));
}
TEST_F(AudioTests, ConcurrentSubmissionsRetainEveryVoice) {
  Open(4);
  std::array<MGDFVoiceId, 4> ids{};
  std::array<std::future<void>, 4> workers;
  for (size_t i = 0; i < workers.size(); ++i)
    workers[i] = std::async(std::launch::async, [&, i] {
      ComObject<IMGDFSoundCommandList> commands;
      ComObject<IMGDFSoundPcmStream> pcm;
      const MGDFSoundPcmDesc pd{2, 48000, 1000};
      if (FAILED(Manager->CreateCommandList(commands.Assign())) ||
          FAILED(Manager->CreatePcmStream(&pd, pcm.Assign())))
        return;
      pcm->WriteSilence(1000);
      const auto desc = Desc();
      ids[i] = commands->PlayPcm(pcm, &desc);
      Manager->Submit(commands);
    });
  for (auto &worker : workers) {
    ASSERT_EQ(std::future_status::ready, worker.wait_for(2s));
    worker.get();
  }
  Manager->Update();
  for (auto id : ids) {
    EXPECT_NE(0U, id);
    EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  }
  EXPECT_EQ(4U, Stats().RealVoices);
}
TEST_F(AudioTests, PlayPositionPublishesAConsistentFrameAndTimePair) {
  auto pcm = Pcm(48000, 100000);
  pcm->WriteSilence(100000);
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  std::atomic<bool> done{false}, bad{false};
  std::promise<void> ready;
  auto readyFuture = ready.get_future();
  auto reader = std::async(std::launch::async, [&] {
    ready.set_value();
    while (!done.load()) {
      UINT64 frame = 0;
      double time = 0;
      pcm->GetPlayPosition(&frame, &time);
      if (std::abs(static_cast<double>(frame) - time * 48000) > 0.001)
        bad.store(true);
    }
  });
  const auto started = readyFuture.wait_for(2s);
  if (started != std::future_status::ready) done.store(true);
  ASSERT_EQ(std::future_status::ready, started);
  for (int i = 0; i < 100; ++i) Render(240);
  done.store(true);
  ASSERT_EQ(std::future_status::ready, reader.wait_for(2s));
  reader.get();
  EXPECT_FALSE(bad.load());
}

class AudioIntegrationTests : public AudioTests {
 protected:
  void SetUp() override {
    AudioTests::SetUp();
    wchar_t folder[MAX_PATH]{}, path[MAX_PATH]{};
    ASSERT_NE(0U, GetTempPathW(MAX_PATH, folder));
    ASSERT_NE(0U, GetTempFileNameW(folder, L"m1a", 0, path));
    Root = path;
    std::filesystem::remove(Root);
    std::filesystem::create_directory(Root);
  }
  void TearDown() override {
    AudioTests::TearDown();
    std::filesystem::remove_all(Root);
  }
  ComObject<IMGDFReadOnlyFile> File(const std::filesystem::path &path) {
    auto context = std::make_shared<vfs::ReadOnlyFileContext>();
    context->RootPath = path.parent_path();
    return MakeCom<vfs::DefaultReadOnlyFileImpl>(path.filename().wstring(),
                                                 path, context)
        .As<IMGDFReadOnlyFile>();
  }
  ComObject<IMGDFReadOnlyFile> Wav(const std::vector<float> &pcm) {
    const auto path = Root / L"sample.wav";
    std::ofstream file(path, std::ios::binary);
    const UINT32 size = static_cast<UINT32>(pcm.size() * sizeof(float));
    const auto u32 = [&](UINT32 value) {
      file.write(reinterpret_cast<const char *>(&value), 4);
    };
    const auto u16 = [&](UINT16 value) {
      file.write(reinterpret_cast<const char *>(&value), 2);
    };
    file.write("RIFF", 4);
    u32(size + 36);
    file.write("WAVEfmt ", 8);
    u32(16);
    u16(3);
    u16(2);
    u32(48000);
    u32(48000 * 8);
    u16(8);
    u16(32);
    file.write("data", 4);
    u32(size);
    file.write(reinterpret_cast<const char *>(pcm.data()), size);
    file.close();
    return File(path);
  }
  ComObject<IMGDFReadOnlyFile> Ogg() {
    const auto path = Root / L"sample.ogg";
    std::ofstream file(path, std::ios::binary);
    vorbis_info info{};
    vorbis_info_init(&info);
    EXPECT_EQ(0, vorbis_encode_init_vbr(&info, 2, 48000, 0.2f));
    vorbis_comment comment{};
    vorbis_comment_init(&comment);
    vorbis_dsp_state dsp{};
    EXPECT_EQ(0, vorbis_analysis_init(&dsp, &info));
    vorbis_block block{};
    EXPECT_EQ(0, vorbis_block_init(&dsp, &block));
    ogg_stream_state stream{};
    EXPECT_EQ(0, ogg_stream_init(&stream, 42));
    const auto flush = [&] {
      ogg_page page{};
      while (ogg_stream_flush(&stream, &page)) {
        file.write(reinterpret_cast<const char *>(page.header),
                   page.header_len);
        file.write(reinterpret_cast<const char *>(page.body), page.body_len);
      }
    };
    ogg_packet header{}, comments{}, code{};
    EXPECT_EQ(0, vorbis_analysis_headerout(&dsp, &comment, &header, &comments,
                                           &code));
    ogg_stream_packetin(&stream, &header);
    ogg_stream_packetin(&stream, &comments);
    ogg_stream_packetin(&stream, &code);
    flush();
    auto **channels = vorbis_analysis_buffer(&dsp, 480);
    const auto signal = Signal(480);
    for (size_t i = 0; i < 480; ++i) {
      channels[0][i] = signal[2 * i];
      channels[1][i] = signal[2 * i + 1];
    }
    vorbis_analysis_wrote(&dsp, 480);
    vorbis_analysis_wrote(&dsp, 0);
    while (vorbis_analysis_blockout(&dsp, &block) == 1) {
      vorbis_analysis(&block, nullptr);
      vorbis_bitrate_addblock(&block);
      ogg_packet packet{};
      while (vorbis_bitrate_flushpacket(&dsp, &packet)) {
        ogg_stream_packetin(&stream, &packet);
        flush();
      }
    }
    ogg_stream_clear(&stream);
    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    vorbis_comment_clear(&comment);
    vorbis_info_clear(&info);
    file.close();
    return File(path);
  }
  ComObject<IMGDFSoundSamples> Load(
      IMGDFReadOnlyFile *file, const MGDFSoundSamplesDesc *desc = nullptr) {
    ComObject<IMGDFSoundSamples> samples;
    EXPECT_EQ(S_OK, Manager->CreateSoundSamples(file, desc, samples.Assign()));
    EXPECT_TRUE(Manager->WaitForLoads(2s));
    Manager->Update();
    return samples;
  }
  std::filesystem::path Root;
};
TEST_F(AudioIntegrationTests,
       WavSegmentsCopyTheDescriptorAndSelectTheRequestedFrames) {
  const auto signal = Signal(1000);
  auto file = Wav(signal);
  MGDFSoundSegment segment{0.005, 0.005, FALSE};
  const MGDFSoundSamplesDesc desc{&segment, 1};
  ComObject<IMGDFSoundSamples> samples;
  ASSERT_EQ(S_OK, Manager->CreateSoundSamples(file, &desc, samples.Assign()));
  segment.Start = 100;
  ASSERT_TRUE(Manager->WaitForLoads(2s));
  Manager->Update();
  ASSERT_TRUE(samples->IsLoaded());
  EXPECT_NEAR(0.005, samples->GetDuration(0), 1e-9);
  const auto voice = Desc();
  Commands->Play(samples, &voice);
  Submit();
  const auto output = Render(100);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[480 + i], output[i], 1e-6f) << i;
}
TEST_F(AudioIntegrationTests,
       SegmentOutsideTheFileFailsAndEndsItsPendingVoice) {
  auto file = Wav(Signal(1000));
  const MGDFSoundSegment segment{100, 1, FALSE};
  const MGDFSoundSamplesDesc desc{&segment, 1};
  ComObject<IMGDFSoundSamples> samples;
  ASSERT_EQ(S_OK, Manager->CreateSoundSamples(file, &desc, samples.Assign()));
  const auto voice = Desc();
  const auto id = Commands->Play(samples, &voice);
  Submit();
  ASSERT_TRUE(Manager->WaitForLoads(2s));
  Manager->Update();
  EXPECT_TRUE(samples->HasFailed());
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
}
TEST_F(AudioIntegrationTests, VorbisFileDecodesOnTheSampleWorker) {
  auto file = Ogg();
  auto samples = Load(file);
  EXPECT_TRUE(samples->IsLoaded());
  EXPECT_DOUBLE_EQ(0.01, samples->GetDuration(0));
}
TEST_F(AudioIntegrationTests, VirtualLoopRetainsItsPlayPosition) {
  Open(1);
  const auto signal = Signal(1000);
  auto file = Wav(signal);
  const MGDFSoundSegment segment{0, 0, TRUE};
  const MGDFSoundSamplesDesc sd{&segment, 1};
  auto samples = Load(file, &sd);
  ASSERT_TRUE(samples->IsLoaded());
  auto desc = Desc();
  const auto loop = Commands->Play(samples, &desc);
  auto blocker = Pcm();
  blocker->WriteSilence(4000);
  desc.Priority = 1;
  const auto other = Commands->PlayPcm(blocker, &desc);
  Submit();
  Render(1240);
  Commands->Stop(other, 0);
  Submit();
  Render(64);  // OpenAL fades in a source that starts at an offset.
  const auto output = Render(100);
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(loop));
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[608 + i], output[i], 1e-6f) << i;
}
TEST_F(AudioIntegrationTests, LateSampleStartUsesTheElapsedOffset) {
  const auto signal = Signal(2000);
  auto file = Wav(signal);
  auto samples = Load(file);
  ASSERT_TRUE(samples->IsLoaded());
  Render(480);
  auto desc = Desc();
  desc.StartTime = 0.005;
  Commands->Play(samples, &desc);
  Submit();
  Render(64);  // OpenAL fades in a source that starts at an offset.
  const auto output = Render(100);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[608 + i], output[i], 1e-6f) << i;
}
TEST_F(AudioTests, PauseKeepsRealPcmFramesUntilResume) {
  auto pcm = Pcm();
  pcm->WriteSilence(4000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  Commands->SetBusPaused(desc.Bus, TRUE);
  Submit();
  Render(480);
  EXPECT_EQ(MGDF_VOICE_PAUSED, Manager->GetVoiceStatus(id));
  EXPECT_EQ(480U, Position(pcm));
  Commands->SetBusPaused(desc.Bus, FALSE);
  Submit();
  Render(480);
  EXPECT_EQ(960U, Position(pcm));
}
TEST_F(AudioTests, StopFadeEndsAtItsDeadline) {
  auto pcm = Pcm();
  pcm->WriteSilence(4000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Commands->Stop(id, 0.01f);
  Submit();
  Render(240);
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  Render(240);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
}
TEST_F(AudioTests, VolumeRampReachesItsTarget) {
  auto pcm = Pcm();
  const std::vector<float> signal(8000, 0.25f);
  pcm->Write(signal.data(), 4000);
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  Commands->SetVolume(id, 0, 0.01f);
  Submit();
  Render(240);
  Render(64);
  const auto middle = Render(16);
  EXPECT_NEAR(0.125f, middle[0], 1e-6f);
  Render(160);
  Render(64);
  const auto end = Render(16);
  for (const auto sample : end) EXPECT_FLOAT_EQ(0, sample);
}
TEST_F(AudioTests, PcmRingWrapPreservesTheSignal) {
  auto pcm = Pcm(48000, 100);
  const auto first = Signal(100);
  ASSERT_EQ(100U, pcm->Write(first.data(), 100));
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(50);
  EXPECT_EQ(74U, pcm->GetWritableFrames());
  const std::vector<float> second(148, 0.125f);
  EXPECT_EQ(74U, pcm->Write(second.data(), 80));
  const auto output = Render(100);
  for (size_t i = 0; i < 100; ++i) EXPECT_FLOAT_EQ(first[100 + i], output[i]);
  for (size_t i = 100; i < output.size(); ++i)
    EXPECT_FLOAT_EQ(0.125f, output[i]);
}
TEST_F(AudioTests, VoiceAndBusHighFrequencyGainsReduceHighFrequencies) {
  auto pcm = Pcm();
  std::vector<float> signal(8000);
  for (size_t i = 0; i < signal.size() / 2; ++i) {
    const float value = (i & 1) ? -0.25f : 0.25f;
    signal[2 * i] = signal[2 * i + 1] = value;
  }
  pcm->Write(signal.data(), 4000);
  auto desc = Desc();
  desc.GainHF = 0.5f;
  Commands->SetBusGainHF(desc.Bus, 0.5f, 0);
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Render(480);
  const auto output = Render(480);
  double energy = 0;
  for (const auto sample : output) energy += sample * sample;
  EXPECT_LT(energy / output.size(), 0.005);
  EXPECT_GT(energy / output.size(), 0.00001);
}
TEST_F(AudioIntegrationTests, SampleRmsSelectsTheLouderVoice) {
  Open(1);
  auto quietFile = Wav(std::vector<float>(2000, 0.01f));
  auto quiet = Load(quietFile);
  auto loudFile = Wav(std::vector<float>(2000, 0.25f));
  auto loud = Load(loudFile);
  ASSERT_TRUE(quiet->IsLoaded());
  ASSERT_TRUE(loud->IsLoaded());
  auto desc = Desc();
  desc.Audibility = 0;
  const auto first = Commands->Play(quiet, &desc);
  const auto second = Commands->Play(loud, &desc);
  Submit();
  EXPECT_EQ(MGDF_VOICE_VIRTUAL, Manager->GetVoiceStatus(first));
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(second));
}
TEST_F(AudioIntegrationTests, VirtualOneShotEndsAtTheSegmentEnd) {
  Open(1);
  BlockSource();
  auto file = Wav(Signal(480));
  auto samples = Load(file);
  ASSERT_TRUE(samples->IsLoaded());
  const auto desc = Desc();
  const auto id = Commands->Play(samples, &desc);
  Submit();
  EXPECT_EQ(MGDF_VOICE_VIRTUAL, Manager->GetVoiceStatus(id));
  Render(480);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
}
}  // namespace
