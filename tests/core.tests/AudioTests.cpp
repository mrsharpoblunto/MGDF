#include "stdafx.h"
#undef min
#undef max
#include <vorbis/vorbisenc.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <numbers>
#include <thread>

#include "../../src/core/audio/openal/OpenALSoundManagerComponentImpl.hpp"
#include "../../src/core/common/MGDFPreferenceSet.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponent.hpp"
#include "AudioOutputTestEndpoint.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::audio::openal_audio;
using namespace std::chrono_literals;
namespace {
void Await(std::future<void> &task) {
  if (task.wait_for(2s) != std::future_status::ready) {
    ADD_FAILURE() << "Audio task did not finish within two seconds";
    // A blocked task would also block its future destructor.
    std::_Exit(EXIT_FAILURE);
  }
  task.get();
}
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
TEST_F(AudioTests, PlayingPcmRetainsTheLastModuleReferenceUntilItEnds) {
  auto pcm = Pcm();
  const auto signal = Signal(1000);
  ASSERT_EQ(1000U, pcm->Write(signal.data(), 1000));
  pcm->End();
  const auto desc = Desc();
  const auto id = Commands->PlayPcm(pcm, &desc);
  Submit();
  auto release = std::async(std::launch::async, [&] { pcm.Clear(); });
  Await(release);
  const auto output = Render(480);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[i], output[i], 1e-6f) << i;
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  Render(1000);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
}
TEST_F(AudioTests, PcmRemainsValidAfterShutdownAndCreationAborts) {
  auto pcm = Pcm();
  pcm->WriteSilence(1000);
  const auto desc = Desc();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  Manager->Stop();
  ComObject<IMGDFSoundPcmStream> rejected;
  const MGDFSoundPcmDesc pcmDesc{2, 48000, 1000};
  EXPECT_EQ(E_ABORT, Manager->CreatePcmStream(&pcmDesc, rejected.Assign()));
  EXPECT_FALSE(rejected);
  Manager.Clear();
  EXPECT_EQ(48000U, pcm->GetSampleRate());
  EXPECT_EQ(0U, pcm->WriteSilence(1));
  // A retained list can still release its control and stream after shutdown.
  Commands->PlayPcm(pcm, &desc);
  pcm.Clear();
  Commands.Clear();
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
    Await(worker);
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
  Await(readyFuture);
  for (int i = 0; i < 100; ++i) Render(240);
  done.store(true);
  Await(reader);
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
  ComObject<IMGDFReadOnlyFile> Ogg(bool loop = false) {
    const auto path = Root / L"sample.ogg";
    std::ofstream file(path, std::ios::binary);
    vorbis_info info{};
    vorbis_info_init(&info);
    EXPECT_EQ(0, vorbis_encode_init_vbr(&info, 2, 48000, 0.2f));
    vorbis_comment comment{};
    vorbis_comment_init(&comment);
    if (loop) {
      vorbis_comment_add_tag(&comment, "LOOPSTART", "120");
      vorbis_comment_add_tag(&comment, "LOOPLENGTH", "240");
    }
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
TEST_F(AudioIntegrationTests,
       SampleVoiceRetainsTheLastModuleReferenceUntilItEnds) {
  const auto signal = Signal(1000);
  auto samples = Load(Wav(signal));
  const auto desc = Desc();
  const auto id = Commands->Play(samples, &desc);
  // The recorded command must retain the samples before the voice exists.
  auto release = std::async(std::launch::async, [&] { samples.Clear(); });
  Await(release);
  Submit();
  const auto output = Render(480);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_NEAR(signal[i], output[i], 1e-6f) << i;
  EXPECT_EQ(MGDF_VOICE_PLAYING, Manager->GetVoiceStatus(id));
  Render(1000);
  EXPECT_EQ(MGDF_VOICE_ENDED, Manager->GetVoiceStatus(id));
  Manager->Update();
}
TEST_F(AudioIntegrationTests,
       SamplesRemainValidAfterShutdownAndCreationAborts) {
  auto file = Wav(Signal(1000));
  auto samples = Load(file);
  const auto desc = Desc();
  Commands->Play(samples, &desc);
  Submit();
  Manager->Stop();
  ComObject<IMGDFSoundSamples> rejected;
  EXPECT_EQ(E_ABORT,
            Manager->CreateSoundSamples(file, nullptr, rejected.Assign()));
  EXPECT_FALSE(rejected);
  Manager.Clear();
  EXPECT_TRUE(samples->IsLoaded());
  EXPECT_EQ(1U, samples->GetSegmentCount());
  EXPECT_DOUBLE_EQ(1000.0 / 48000, samples->GetDuration(0));
  Commands->Play(samples, &desc);
  samples.Clear();
  Commands.Clear();
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
  EXPECT_LT(energy / output.size(), 0.001);
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
class AudioStreamIntegrationTests : public AudioIntegrationTests {
 protected:
  ComObject<IMGDFSoundStream> Stream(IMGDFReadOnlyFile *file) {
    ComObject<IMGDFSoundStream> stream;
    EXPECT_EQ(S_OK, Manager->CreateSoundStream(file, stream.Assign()));
    return stream;
  }
  void Sync() {
    if (!Manager->WaitForStreams(2s)) {
      ADD_FAILURE() << "Stream worker did not finish within two seconds";
      std::_Exit(EXIT_FAILURE);
    }
    Manager->Update();
    if (!Manager->WaitForStreams(2s)) {
      ADD_FAILURE() << "Stream snapshot did not finish within two seconds";
      std::_Exit(EXIT_FAILURE);
    }
  }
  std::vector<float> Step(UINT32 frames) {
    auto output = Render(frames);
    Sync();
    return output;
  }
};

TEST_F(AudioStreamIntegrationTests,
       ReleasedStreamRetainsPcmUntilItsVoiceStops) {
  const auto signal = Signal(2000);
  auto stream = Stream(Wav(signal));
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  auto release = std::async(std::launch::async, [&] { stream.Clear(); });
  Await(release);
  ASSERT_TRUE(Manager->WaitForStreams(2s));
  // The worker has dropped its PCM reference. The mixer still owns the voice.
  const auto output = Render(480);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_FLOAT_EQ(signal[i], output[i]) << i;
  EXPECT_EQ(0U, Stats().Streams);
  Render(64);  // Drain OpenAL's stop de-click tail.
  const auto silence = Render(240);
  EXPECT_TRUE(std::all_of(silence.begin(), silence.end(),
                          [](float x) { return x == 0; }));
}

TEST_F(AudioStreamIntegrationTests,
       StreamRemainsValidAfterShutdownAndCreationAborts) {
  auto file = Wav(Signal(2000));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  Manager->Stop();
  ComObject<IMGDFSoundStream> rejected;
  EXPECT_EQ(E_ABORT, Manager->CreateSoundStream(file, rejected.Assign()));
  EXPECT_FALSE(rejected);
  Manager.Clear();
  EXPECT_DOUBLE_EQ(2000.0 / 48000, stream->GetDuration());
  UINT64 length = 0;
  ASSERT_EQ(S_OK, stream->GetName(nullptr, &length));
  std::wstring name(length, L'\0');
  ASSERT_EQ(S_OK, stream->GetName(name.data(), &length));
  EXPECT_STREQ(L"sample.wav", name.c_str());
  stream->Play(0);
  stream->SetLoop(0, 0);
  stream->SetBus(MGDF_BUS_MUSIC);
  EXPECT_FALSE(stream->IsPlaying());
  stream.Clear();
}

TEST_F(AudioStreamIntegrationTests, WavLoopPreservesEveryFrameAcrossRefills) {
  const auto signal = Signal(960);
  auto file = Wav(signal);
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  EXPECT_DOUBLE_EQ(0.02, stream->GetDuration());
  UINT64 length = 0;
  ASSERT_EQ(S_OK, stream->GetName(nullptr, &length));
  std::wstring name(length, L'\0');
  ASSERT_EQ(S_OK, stream->GetName(name.data(), &length));
  EXPECT_STREQ(L"sample.wav", name.c_str());
  stream->SetLoop(0.005, 0.015);
  stream->Play(0);
  Sync();
  std::vector<float> expected(signal.begin(), signal.begin() + 1440);
  for (int loop = 0; loop < 210; ++loop)
    expected.insert(expected.end(), signal.begin() + 480,
                    signal.begin() + 1440);
  for (size_t first = 0; first < expected.size(); first += 480) {
    const auto output = Step(240);
    for (size_t i = 0; i < output.size(); ++i)
      ASSERT_FLOAT_EQ(expected[first + i], output[i]) << first + i;
  }
  EXPECT_DOUBLE_EQ(0.005, stream->GetPosition());
  EXPECT_TRUE(stream->IsPlaying());
  EXPECT_EQ(1U, Stats().Streams);
  EXPECT_EQ(0U, Stats().Underruns);
}

TEST_F(AudioStreamIntegrationTests,
       VorbisCommentsSelectExactDecodedLoopFrames) {
  auto file = Ogg(true);
  auto samples = Load(file);
  ASSERT_TRUE(samples->IsLoaded());
  const auto desc = Desc();
  Commands->Play(samples, &desc);
  Submit();
  const auto decoded = Render(480);
  Render(64);  // Drain OpenAL's source-end de-click tail.
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  std::vector<float> expected(decoded.begin(), decoded.begin() + 720);
  for (int loop = 0; loop < 4; ++loop)
    expected.insert(expected.end(), decoded.begin() + 240,
                    decoded.begin() + 720);
  const auto output = Step(1320);
  ASSERT_EQ(expected.size(), output.size());
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_FLOAT_EQ(expected[i], output[i]) << i;
  EXPECT_DOUBLE_EQ(0.0025, stream->GetPosition());
  EXPECT_EQ(0U, Stats().Underruns);
}

TEST_F(AudioStreamIntegrationTests, FadeInReachesFullVolume) {
  auto file = Wav(std::vector<float>(4000, 0.25f));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0.01f);
  Sync();
  const auto first = Step(240);
  EXPECT_FLOAT_EQ(0, first.back());
  Step(64);
  EXPECT_NEAR(0.125f, Step(16)[0], 1e-6f);
  Step(160);
  Step(64);
  EXPECT_FLOAT_EQ(0.25f, Step(16)[0]);
}

TEST_F(AudioStreamIntegrationTests, StopFadesThenResetsAndReplaysFromTheStart) {
  auto file = Wav(std::vector<float>(4000, 0.25f));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  Step(240);
  stream->Stop(0.01f);
  Sync();
  Step(240);
  EXPECT_TRUE(stream->IsPlaying());
  Step(64);
  EXPECT_NEAR(0.125f, Step(16)[0], 1e-6f);
  Step(160);
  EXPECT_FALSE(stream->IsPlaying());
  EXPECT_DOUBLE_EQ(0, stream->GetPosition());
  Step(64);  // Drain OpenAL's stop de-click tail.
  const auto silence = Step(240);
  EXPECT_TRUE(std::all_of(silence.begin(), silence.end(),
                          [](float x) { return x == 0; }));
  stream->Play(0);
  Sync();
  EXPECT_FLOAT_EQ(0.25f, Step(240)[0]);
  EXPECT_DOUBLE_EQ(0.005, stream->GetPosition());
}

TEST_F(AudioStreamIntegrationTests, PauseFadeHoldsPositionAndResumeContinues) {
  auto file = Wav(std::vector<float>(8000, 0.25f));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  Step(240);
  stream->Pause(0.01f);
  Sync();
  Step(240);
  EXPECT_TRUE(stream->IsPlaying());
  Step(64);
  EXPECT_NEAR(0.125f, Step(16)[0], 1e-6f);
  Step(160);
  EXPECT_FALSE(stream->IsPlaying());
  EXPECT_DOUBLE_EQ(0.015, stream->GetPosition());
  Step(64);  // Drain OpenAL's pause de-click tail.
  const auto silence = Step(960);
  EXPECT_TRUE(std::all_of(silence.begin(), silence.end(),
                          [](float x) { return x == 0; }));
  EXPECT_DOUBLE_EQ(0.015, stream->GetPosition());
  stream->Play(0.005f);
  Sync();
  Step(240);
  Step(64);
  EXPECT_FLOAT_EQ(0.25f, Step(16)[0]);
  EXPECT_NEAR(0.015 + 320.0 / 48000, stream->GetPosition(), 1e-9);
}

TEST_F(AudioStreamIntegrationTests, VolumeRampReachesItsTarget) {
  auto file = Wav(std::vector<float>(4000, 0.25f));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  Step(240);
  stream->SetVolume(0.25f, 0.01f);
  Sync();
  Step(240);
  Step(64);
  EXPECT_NEAR(0.15625f, Step(16)[0], 1e-6f);
  Step(160);
  Step(64);
  EXPECT_FLOAT_EQ(0.0625f, Step(16)[0]);
}

TEST_F(AudioStreamIntegrationTests, MusicAndSelectedBusVolumeApply) {
  auto file = Wav(std::vector<float>(8000, 0.25f));
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  Commands->SetBusVolume(Manager->GetBus(MGDF_BUS_MUSIC), 0.5f, 0);
  Submit();
  stream->Play(0);
  Sync();
  EXPECT_FLOAT_EQ(0.125f, Step(240)[0]);
  const auto child = Bus(Manager->GetBus(MGDF_BUS_MASTER), 0.25f);
  stream->SetBus(child);
  Sync();
  Step(64);
  EXPECT_FLOAT_EQ(0.0625f, Step(240)[0]);
}

TEST_F(AudioStreamIntegrationTests,
       SubmittingThreadCanStallForOneSecondOfAudio) {
  const auto signal = Signal(96000);
  auto file = Wav(signal);
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  std::promise<void> submitted, resume;
  auto ready = submitted.get_future();
  auto resumed = resume.get_future();
  auto submitter = std::async(std::launch::async, [&] {
    stream->Play(0);
    submitted.set_value();
    Await(resumed);
  });
  Await(ready);
  Sync();
  const auto output = Render(48000);
  resume.set_value();
  Await(submitter);
  Sync();
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_FLOAT_EQ(signal[i], output[i]) << i;
  EXPECT_DOUBLE_EQ(1, stream->GetPosition());
  EXPECT_TRUE(stream->IsPlaying());
  EXPECT_EQ(0U, Stats().Underruns);
}

TEST_F(AudioStreamIntegrationTests, BadFileClearsTheOutputAndReturnsFailure) {
  const auto path = Root / L"bad.wav";
  {
    std::ofstream file(path, std::ios::binary);
    file << "not an audio file";
  }
  auto file = File(path);
  IMGDFSoundStream *stream = reinterpret_cast<IMGDFSoundStream *>(1);
  EXPECT_EQ(E_FAIL, Manager->CreateSoundStream(file, &stream));
  EXPECT_EQ(nullptr, stream);
  EXPECT_EQ(E_POINTER, Manager->CreateSoundStream(file, nullptr));
  EXPECT_EQ(E_INVALIDARG, Manager->CreateSoundStream(nullptr, &stream));
  EXPECT_EQ(nullptr, stream);
}

TEST_F(AudioStreamIntegrationTests, EndOfFileStopsWithoutAnUnderrun) {
  const auto signal = Signal(480);
  auto file = Wav(signal);
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  const auto output = Step(480);
  EXPECT_EQ(signal, output);
  EXPECT_FALSE(stream->IsPlaying());
  EXPECT_DOUBLE_EQ(0.01, stream->GetPosition());
  EXPECT_EQ(0U, Stats().Underruns);
}

TEST_F(AudioStreamIntegrationTests, StopAndPlayInOneBatchRestartsTheFile) {
  const auto signal = Signal(2000);
  auto file = Wav(signal);
  auto stream = Stream(file);
  ASSERT_TRUE(stream);
  stream->Play(0);
  Sync();
  Step(240);
  stream->Stop(0);
  stream->Play(0);
  Sync();
  Step(64);  // The old source has a short de-click tail.
  const auto output = Step(240);
  for (size_t i = 0; i < output.size(); ++i)
    ASSERT_FLOAT_EQ(signal[128 + i], output[i]);
  EXPECT_DOUBLE_EQ(304.0 / 48000, stream->GetPosition());
}
}  // namespace

namespace {
std::filesystem::path OutputTestFile() {
  return std::filesystem::temp_directory_path() /
         ("mgdf_output_" + std::to_string(GetCurrentProcessId()) + ".wav");
}
template <typename Complete>
bool WaitForOutput(audio::AudioCaptureRing *ring, Complete complete,
                   audio::AudioBlock *last = nullptr) {
  const auto deadline = GetTickCount64() + 10000;
  for (;;) {
    if (GetTickCount64() > deadline) return false;
    audio::AudioBlock block;
    while (ring->Pop(block)) {
      if (last) {
        EXPECT_GE(block.PlayTime,
                  last->PlayTime + last->Frames * audio::AudioTicksPerSecond /
                                       audio::MixRate);
        *last = block;
      }
    }
    if (complete()) return true;
    if (WaitForSingleObject(ring->Ready, 1000) != WAIT_OBJECT_0) return false;
  }
}
bool WaitForMix(audio::AudioCaptureRing *ring,
                OpenALSoundManagerComponentImpl *manager, double time,
                audio::AudioBlock *last = nullptr) {
  return WaitForOutput(
      ring, [&] { return manager->GetAudioTime() >= time; }, last);
}
}  // namespace
TEST_F(AudioTests, FileOutputEqualsTheLoopbackMix) {
  Commands.Clear();
  Manager.Clear();
  const auto path = OutputTestFile();
  Manager = MakeCom<OpenALSoundManagerComponentImpl>(
      AudioOptions{.Sources = 2, .OutputFile = path});
  ASSERT_EQ(S_OK, Manager->Init());
  auto ring = Manager->CaptureSource()->Start();
  auto pcm = Pcm(48000, 9600);
  const auto signal = Signal(4800);
  pcm->Write(signal.data(), 4800);
  pcm->End();
  auto desc = Desc();
  ASSERT_EQ(S_OK, Manager->CreateCommandList(Commands.Assign()));
  const auto voice = Commands->PlayPcm(pcm, &desc);
  Manager->Submit(Commands);
  ASSERT_TRUE(WaitForOutput(ring, [&] {
    return Manager->GetVoiceStatus(voice) == MGDF_VOICE_ENDED;
  }));
  Manager->Stop();
  std::ifstream file(path, std::ios::binary);
  ASSERT_TRUE(file.good());
  std::array<UINT32, 11> header{};
  file.read(reinterpret_cast<char *>(header.data()), sizeof(header));
  ASSERT_EQ(48000u, header[6]);
  ASSERT_EQ((2u << 16) | 3u, header[5]);
  ASSERT_GT(header[10], 0u);
  std::vector<float> recorded(header[10] / sizeof(float));
  file.read(reinterpret_cast<char *>(recorded.data()), header[10]);
  ASSERT_TRUE(file.good());
  file.close();
  std::filesystem::remove(path);
  // Command setup can take any amount of time. Reproduce its actual start
  // frame, then compare every WAV sample, including the silence around it.
  const auto first = std::find_if(recorded.begin(), recorded.end(),
                                  [](float value) { return value != 0; });
  ASSERT_NE(recorded.end(), first);
  ASSERT_EQ(0, std::distance(recorded.begin(), first) % 2);
  desc.StartTime = double(std::distance(recorded.begin(), first) / 2) / 48000;
  Open(2);
  pcm = Pcm(48000, 9600);
  pcm->Write(signal.data(), 4800);
  pcm->End();
  Commands->PlayPcm(pcm, &desc);
  Submit();
  std::vector<float> expected;
  while (expected.size() < recorded.size()) {
    const auto count = static_cast<UINT32>(
        (std::min)(size_t(240), (recorded.size() - expected.size()) / 2));
    const auto block = Render(count);
    expected.insert(expected.end(), block.begin(), block.end());
  }
  ASSERT_EQ(expected.size(), recorded.size());
  for (size_t i = 0; i < expected.size(); ++i)
    ASSERT_NEAR(expected[i], recorded[i], 1e-6f) << i;
}
TEST_F(AudioTests, EndpointChangesPreserveTheClockAndPcmPosition) {
  auto source = MakeCom<TestOutputSource>();
  source->State->QueuedFrames = 2880;
  Commands.Clear();
  Manager.Clear();
  AudioOptions options{.Sources = 2};
  options.EndpointSource = source;
  Manager = MakeCom<OpenALSoundManagerComponentImpl>(options);
  ASSERT_EQ(S_OK, Manager->Init());
  auto ring = Manager->CaptureSource()->Start();
  auto pcm = Pcm(48000, 192000);
  ASSERT_EQ(192000u, pcm->WriteSilence(192000));
  pcm->End();
  ASSERT_EQ(S_OK, Manager->CreateCommandList(Commands.Assign()));
  auto desc = Desc();
  const auto voice = Commands->PlayPcm(pcm, &desc);
  Manager->Submit(Commands);
  audio::AudioBlock last{};
  // Let the initial retry deadline pass while an endpoint is active.
  ASSERT_TRUE(WaitForMix(ring, Manager, 1.1, &last));
  UINT64 firstPlayed = 0;
  double firstTime = 0;
  ASSERT_TRUE(WaitForOutput(
      ring,
      [&] {
        pcm->GetPlayPosition(&firstPlayed, &firstTime);
        return firstPlayed != 0;
      },
      &last));
  const auto start = firstTime - double(firstPlayed) / 48000;
  EXPECT_EQ(0u, Stats().DeviceChanges);
  struct ResumeOnExit {
    HANDLE Resume;
    ~ResumeOnExit() { SetEvent(Resume); }
  } resume{source->State->Resume};
  double previous = Manager->GetAudioTime();
  UINT64 changes = 0;
  for (const UINT32 channels : {6u, 0u, 2u, 2u}) {
    SCOPED_TRACE(channels);
    const auto opens = source->State->OpenCalls.load();
    if (channels == 2) source->State->QueuedFrames = 0;
    ResetEvent(source->State->Resume);
    ResetEvent(source->State->Opened);
    source->Change(channels);
    ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(source->State->Opened, 2000));
    // Mixing continues during the open. A retry must not discard its result.
    ASSERT_TRUE(WaitForMix(ring, Manager, previous + 0.025, &last));
    SetEvent(source->State->Resume);
    if (changes < 3) ++changes;
    ASSERT_TRUE(WaitForOutput(
        ring, [&] { return Stats().DeviceChanges >= changes; }, &last));
    ASSERT_TRUE(
        WaitForMix(ring, Manager, Manager->GetAudioTime() + 0.1, &last));
    // An absent endpoint can retry again while the test thread is suspended.
    if (channels) EXPECT_EQ(opens + 1, source->State->OpenCalls.load());
    EXPECT_EQ(changes, Stats().DeviceChanges);
    if (!channels) {
      ASSERT_TRUE(WaitForOutput(
          ring, [&] { return source->State->OpenCalls.load() > opens + 1; },
          &last));
      EXPECT_EQ(changes, Stats().DeviceChanges);
    }
    const double now = Manager->GetAudioTime();
    EXPECT_GE(now, previous);
    UINT64 played = 0;
    double audioTime = 0;
    pcm->GetPlayPosition(&played, &audioTime);
    EXPECT_NEAR(double(played) / 48000, (std::min)(4.0, audioTime - start),
                1.0 / 48000);
    EXPECT_NE(MGDF_VOICE_PENDING, Manager->GetVoiceStatus(voice));
    previous = now;
  }
  EXPECT_EQ(3u, Stats().DeviceChanges);
  EXPECT_EQ(0u, Stats().Underruns);
  Manager->Stop();
  ASSERT_TRUE(WaitForOutput(ring, [] { return true; }, &last));
  // The final snapshot cannot be ahead of all rendered frames. No limit on
  // snapshot age is needed while the two threads run at different speeds.
  EXPECT_LE(std::llround(Manager->GetAudioTime() * 48000),
            static_cast<int64_t>(last.FirstFrame + last.Frames +
                                 ring->Counts->Dropped.load() *
                                     audio::MixBlockFrames));
}
