#pragma once

#include <al.h>
#include <alc.h>

#include <stack>
#include <unordered_map>

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

class OpenALSoundSystem {
 public:
  virtual ~OpenALSoundSystem(void);

  MGDFError AcquireSource(ALuint *source);
  void ReleaseSource(ALuint source);

 protected:
  OpenALSoundSystem(void);
  virtual MGDFError Init();

  size_t GetFreeSources() const { return _freeSources.size(); }

 private:
  std::unordered_map<ALuint, bool> _allocatedSources;
  std::stack<ALuint> _freeSources;

  ALCcontext *_context;
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF
