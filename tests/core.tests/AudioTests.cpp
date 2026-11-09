#include "stdafx.h"

#include <AL/alext.h>

#include <algorithm>
#include <filesystem>

#include "../../src/core/audio/openal/OpenALSoundManagerComponentImpl.hpp"
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::audio::openal_audio;

namespace {
ALCcontext *ALC_APIENTRY CreateLoopbackContext(ALCdevice *device,
                                               const ALCint *) noexcept {
  const ALCint attributes[]{ALC_FORMAT_CHANNELS_SOFT,
                            ALC_STEREO_SOFT,
                            ALC_FORMAT_TYPE_SOFT,
                            ALC_FLOAT_SOFT,
                            ALC_FREQUENCY,
                            44100,
                            0};
  return alcCreateContext(device, attributes);
}

struct FailedBind {
  static inline int Device, Context;
  static inline UINT32 Destroys, Closes;
  static ALCdevice *ALC_APIENTRY Open(const ALCchar *) noexcept {
    return reinterpret_cast<ALCdevice *>(&Device);
  }
  static ALCcontext *ALC_APIENTRY Create(ALCdevice *, const ALCint *) noexcept {
    return reinterpret_cast<ALCcontext *>(&Context);
  }
  static ALCboolean ALC_APIENTRY Bind(ALCcontext *) noexcept {
    return ALC_FALSE;
  }
  static void ALC_APIENTRY Destroy(ALCcontext *) noexcept { ++Destroys; }
  static ALCboolean ALC_APIENTRY Close(ALCdevice *) noexcept {
    ++Closes;
    return ALC_TRUE;
  }
};
}  // namespace

TEST(AudioTests, FailedContextBindReleasesEachHandleOnce) {
  FailedBind::Destroys = FailedBind::Closes = 0;
  {
    OpenALSoundSystem system({FailedBind::Open, FailedBind::Create,
                              FailedBind::Bind, FailedBind::Destroy,
                              FailedBind::Close});
    EXPECT_EQ(E_FAIL, system.Init());
  }
  EXPECT_EQ(1U, FailedBind::Destroys);
  EXPECT_EQ(1U, FailedBind::Closes);
}

TEST(AudioIntegrationTests, UnplayedStreamDoesNotAdvanceDuringUpdate) {
  Resources::Instance(GetModuleHandleW(nullptr));
  const auto open = reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(
      alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
  const auto render = reinterpret_cast<LPALCRENDERSAMPLESSOFT>(
      alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));
  ASSERT_NE(nullptr, open);
  ASSERT_NE(nullptr, render);
  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> vfs;
  ASSERT_TRUE(vfs::CreateReadOnlyVirtualFileSystemComponent(vfs));
  const auto root = std::filesystem::path(Resources::Instance().RootDir())
                        .parent_path()
                        .parent_path()
                        .parent_path()
                        .parent_path();
  ASSERT_TRUE(vfs->Mount((root / "content/game/content").c_str()));
  const auto manager = MakeCom<OpenALSoundManagerComponentImpl>(
      vfs.As<IMGDFReadOnlyVirtualFileSystem>(),
      OpenALContextFunctions{open, CreateLoopbackContext});
  ASSERT_EQ(S_OK, manager->Init());
  manager->SetStreamVolume(1.0f);
  ComObject<IMGDFReadOnlyFile> file;
  ASSERT_TRUE(vfs->GetFile(L"Stream.ogg", file.Assign()));
  ComObject<IMGDFSoundStream> stream;
  ASSERT_EQ(S_OK, manager->CreateSoundStream(file, stream.Assign()));
  const auto device = alcGetContextsDevice(alcGetCurrentContext());
  const auto before = stream->GetPosition();
  manager->Update();
  std::vector<float> samples(8192 * 2);
  render(device, samples.data(), 8192);
  EXPECT_FALSE(stream->IsPlaying());
  EXPECT_EQ(before, stream->GetPosition());
  EXPECT_TRUE(
      std::ranges::all_of(samples, [](float value) { return value == 0; }));
  ASSERT_EQ(S_OK, stream->Play());
  render(device, samples.data(), 8192);
  EXPECT_TRUE(stream->IsPlaying());
  EXPECT_GT(stream->GetPosition(), before);
}
