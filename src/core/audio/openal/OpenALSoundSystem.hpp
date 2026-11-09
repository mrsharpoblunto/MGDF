#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <stack>
#include <unordered_set>

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

struct OpenALContextFunctions {
  decltype(&alcOpenDevice) OpenDevice = alcOpenDevice;
  decltype(&alcCreateContext) CreateContext = alcCreateContext;
  decltype(&alcMakeContextCurrent) MakeCurrent = alcMakeContextCurrent;
  decltype(&alcDestroyContext) DestroyContext = alcDestroyContext;
  decltype(&alcCloseDevice) CloseDevice = alcCloseDevice;
};

class OpenALSoundSystem {
 public:
  virtual ~OpenALSoundSystem(void);
  explicit OpenALSoundSystem(OpenALContextFunctions contextFunctions = {});
  virtual HRESULT Init();

  HRESULT AcquireSource(ALuint *source);
  void ReleaseSource(ALuint source);

 protected:
  size_t GetFreeSources() const { return _freeSources.size(); }

 private:
  std::unordered_set<ALuint> _allocatedSources;
  std::stack<ALuint> _freeSources;

  const OpenALContextFunctions _contextFunctions;
  ALCcontext *_context;
  bool _alutInitialized = false;
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF
