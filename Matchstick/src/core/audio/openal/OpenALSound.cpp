#include "StdAfx.h"

#include "OpenALSound.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFStringImpl.hpp"
#include "OpenALSoundSystem.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

#pragma warning(disable : 4345)  // disable irrelevant warning about
                                 // initializing POD types via new() syntax.

using namespace DirectX;

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

HRESULT OpenALSound::TryCreate(IMGDFReadOnlyFile *source,
                                 OpenALSoundManagerComponentImpl *manager,
                                 INT32 priority,
                                 ComObject<OpenALSound> &sound) {
  sound = MakeCom<OpenALSound>(manager, priority);
  const auto result = sound->Init(source);
  if (FAILED(result)) {
    sound.Clear();
  }
  return result;
}

OpenALSound::OpenALSound(OpenALSoundManagerComponentImpl *manager,
                         INT32 priority)
    : _soundManager(manager),
      _priority(priority),
      _position(XMFLOAT3(0.0f, 0.0f, 0.0f)),
      _velocity(XMFLOAT3(0.0f, 0.0f, 0.0f)),
      _innerRange(0),
      _outerRange(1),
      _volume(1),
      _attenuationFactor(1),
      _pitch(1),
      _bufferId(0),
      _isActive(false),
      _sourceId(0),
      _isLooping(false),
      _isSourceRelative(true),
      _wasPlaying(false),
      _startPlaying(false) {
  _ASSERTE(manager);
  _globalVolume = manager->GetSoundVolume();
}

HRESULT OpenALSound::Init(IMGDFReadOnlyFile *source) {
  _ASSERTE(source);
  _name = source->GetName();

  const auto result = _soundManager->CreateSoundBuffer(source, &_bufferId);
  if (SUCCEEDED(result)) {
    Reactivate();
  }
  return result;
}

OpenALSound::~OpenALSound() {
  _soundManager->RemoveSound(this);
  _soundManager->RemoveSoundBuffer(_bufferId);
  Deactivate();
}

void OpenALSound::Reactivate() {
  if (SUCCEEDED(_soundManager->AcquireSource(&_sourceId))) {
    alSourcei(_sourceId, AL_BUFFER, _bufferId);
    if (alGetError() != AL_NO_ERROR) {
      LOG("Unable to allocate buffer to audio source", MGDF_LOG_ERROR);
      Deactivate();
    } else {
      _isActive = true;
      SetSourceRelative(_isSourceRelative);
      SetVolume(_volume);
      SetPitch(_pitch);
      SetLooping(_isLooping);
      if (_isLooping && _wasPlaying) {  // resume any looping sample that was
                                        // playing before it was deactivated
        Play();
      }
    }
  }
}

void OpenALSound::Deactivate() {
  if (_isActive) {
    if (_isLooping && IsPlaying()) {
      _wasPlaying = true;
    } else {
      _wasPlaying = false;
    }
    _soundManager->ReleaseSource(_sourceId);
    _startPlaying = false;
    _isActive = false;
  }
}

void OpenALSound::SetSourceRelative(BOOL sourceRelative) {
  _isSourceRelative = sourceRelative;
  if (_isActive) {
    if (_isSourceRelative) {
      alSourcei(_sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
      alSource3f(_sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
      alSource3f(_sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    } else {
      alSourcei(_sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
      alSource3f(_sourceId, AL_POSITION, _position.x, _position.y, _position.z);
      alSource3f(_sourceId, AL_VELOCITY, _velocity.x, _velocity.y, _velocity.z);
    }
  }
}

HRESULT OpenALSound::GetName(wchar_t *name, size_t *length) {
  return CopyWStr(_name, name, length);
}

MGDFSoundPosition *OpenALSound::GetPosition(MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_position, sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *OpenALSound::GetVelocity(MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_velocity, sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *OpenALSound::SetPosition(MGDFSoundPosition *sp) {
  memcpy_s(&_position, sizeof(DirectX::XMFLOAT3), sp, sizeof(MGDFSoundPosition));
  return sp;
}

MGDFSoundPosition *OpenALSound::SetVelocity(MGDFSoundPosition *sp) {
  memcpy_s(&_velocity, sizeof(DirectX::XMFLOAT3), sp, sizeof(MGDFSoundPosition));
  return sp;
}

float OpenALSound::GetInnerRange() { return _innerRange; }

void OpenALSound::SetInnerRange(float innerRange) { _innerRange = innerRange; }

float OpenALSound::GetOuterRange() { return _outerRange; }

void OpenALSound::SetOuterRange(float outerRange) { _outerRange = outerRange; }

BOOL OpenALSound::GetSourceRelative() { return _isSourceRelative; }

float OpenALSound::GetVolume() { return _volume; }

float OpenALSound::GetAttenuatedVolume() const {
  return _volume * _attenuationFactor;
}

void OpenALSound::SetVolume(float volume) {
  _volume = volume;
  if (_isActive) {
    alSourcef(_sourceId, AL_GAIN, _volume * _globalVolume * _attenuationFactor);
  }
}

void OpenALSound::SetGlobalVolume(float globalVolume) {
  _globalVolume = globalVolume;
  if (_isActive) {
    SetVolume(_volume);
  }
}

void OpenALSound::Update(float attenuationFactor) {
  _attenuationFactor = attenuationFactor;

  if (_isActive) {
    SetVolume(_volume);
    alSource3f(_sourceId, AL_POSITION, _position.x, _position.y, _position.z);
    alSource3f(_sourceId, AL_VELOCITY, _velocity.x, _velocity.y, _velocity.z);
    if (_startPlaying) {
      _startPlaying = false;
      alSourcePlay(_sourceId);
    }
  }
}

float OpenALSound::GetPitch() { return _pitch; }

void OpenALSound::SetPitch(float pitch) {
  _pitch = pitch;
  if (_isActive) {
    alSourcef(_sourceId, AL_PITCH, _pitch);
  }
}

void OpenALSound::SetPriority(INT32 priority) { _priority = priority; }

INT32 OpenALSound::GetPriority() { return _priority; }

BOOL OpenALSound::GetLooping() { return _isLooping; }

void OpenALSound::SetLooping(BOOL looping) {
  _isLooping = looping;
  if (_isActive) {
    alSourcei(_sourceId, AL_LOOPING, _isLooping ? AL_TRUE : AL_FALSE);
  }
}

void OpenALSound::Stop() {
  _wasPlaying = false;
  if (_isActive) {
    alSourceStop(_sourceId);
  }
}

void OpenALSound::Pause() {
  _wasPlaying = false;
  if (_isActive) {
    alSourcePause(_sourceId);
  }
}

void OpenALSound::Play() {
  if (_isActive) {
    _startPlaying = true;  // start playing on next update so we can ensure the
                           // position/velocity/attenuation has been calculated
                           // before playing begins
  }
}

BOOL OpenALSound::IsStopped() {
  ALint state;
  alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
  return state == AL_STOPPED;
}

BOOL OpenALSound::IsPaused() {
  ALint state;
  alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
  return state == AL_PAUSED;
}

BOOL OpenALSound::IsPlaying() {
  ALint state;
  alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
  return _startPlaying || state == AL_PLAYING;
}

BOOL OpenALSound::IsActive() { return _isActive; }

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF