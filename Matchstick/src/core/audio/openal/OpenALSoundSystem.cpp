#include "StdAfx.h"

#include "OpenALSoundSystem.hpp"

#include <AL/alut.h>

#include "../../common/MGDFLoggerImpl.hpp"

#define MAX_SOURCES 256

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

/**
initialise the openAL sound
*/
OpenALSoundSystem::OpenALSoundSystem() : _context(nullptr) {}

MGDFError OpenALSoundSystem::Init() {
  ALCdevice *device = alcOpenDevice(nullptr);
  if (device == nullptr) {
    LOG("Unable to open audio device", LOG_ERROR);
    return MGDF_ERR_AUDIO_INIT_FAILED;
  }

  _context = alcCreateContext(device, nullptr);
  if (_context == nullptr) {
    LOG("Failed to initialize OpenAL", LOG_ERROR);
    return MGDF_ERR_AUDIO_INIT_FAILED;
  }

  alcMakeContextCurrent(_context);
  if (alcGetError(device) != AL_NO_ERROR) {
    LOG("Failed to make OpenAL context current", LOG_ERROR);
    return MGDF_ERR_AUDIO_INIT_FAILED;
  }

  if (!alutInitWithoutContext(nullptr, nullptr)) {
    const ALenum alError = alutGetError();
    LOG("Failed to initialize alut" << alutGetErrorString(alError), LOG_ERROR);
    return MGDF_ERR_AUDIO_INIT_FAILED;
  }

  // allocate as many sources as we can (or until we reach a hard coded limit)
  // and add them to the free sources pool.
  do {
    ALuint sourceId;
    alGenSources(1, &sourceId);
    if (alGetError() != AL_NO_ERROR) {
      break;
    } else {
      _freeSources.push(sourceId);
    }
  } while (_freeSources.size() < MAX_SOURCES);

  // check that we could create at least one audio source
  if (!_freeSources.size()) {
    LOG("Unable to create any audio sources", LOG_ERROR);
    return MGDF_ERR_AUDIO_INIT_FAILED;
  }

  LOG("max audio sources determined - " << _freeSources.size(), LOG_LOW);
  LOG("initialised successfully", LOG_LOW);
  return MGDF_OK;
}

OpenALSoundSystem::~OpenALSoundSystem() {
  if (_context) {
    ALCdevice *device = alcGetContextsDevice(_context);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(_context);
    alcCloseDevice(device);

    while (_freeSources.size() > 0) {
      const ALuint source = _freeSources.top();
      _freeSources.pop();
      alDeleteSources(1, &source);
    }

    LOG("uninitialised successfully", LOG_LOW);
  }
}

MGDFError OpenALSoundSystem::AcquireSource(ALuint *source) {
  _ASSERTE(source);
  const size_t freeSources = GetFreeSources();
  if (freeSources > 0) {
    ALuint freeSource = _freeSources.top();
    _freeSources.pop();
    _allocatedSources[freeSource] = true;

    // make sure we clear out any properties from a source acquired from the
    // pool
    alSourcei(freeSource, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(freeSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(freeSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcef(freeSource, AL_PITCH, 1.0f);
    alSourcei(freeSource, AL_LOOPING, AL_FALSE);

    *source = freeSource;
    return MGDF_OK;
  } else {
    LOG("No free sound sources to create stream", LOG_ERROR);
    return MGDF_ERR_NO_FREE_SOURCES;
  }
}

void OpenALSoundSystem::ReleaseSource(ALuint source) {
  auto iter = _allocatedSources.find(source);
  if (iter != _allocatedSources.end()) {
    // stop the source, clear out its buffer and re add it to the free source
    // pool.
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);

    _allocatedSources.erase(iter);
    _freeSources.push(source);
  }
}

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF