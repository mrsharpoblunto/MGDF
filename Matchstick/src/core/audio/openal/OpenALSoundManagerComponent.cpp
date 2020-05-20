#include "StdAfx.h"

#include "OpenALSoundManagerComponent.hpp"

#include <AL/alut.h>
#include <al.h>
#include <alc.h>
#include <limits.h>
#include <math.h>

#include <algorithm>
#include <sstream>

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFPreferenceConstants.hpp"
#include "../../common/MGDFPreferenceSet.hpp"
#include "../../common/MGDFResources.hpp"
#include "OpenALSound.hpp"
#include "VorbisStream.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace DirectX;

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

ComObject<ISoundManagerComponent>
OpenALSoundManagerComponentImpl::CreateOpenALSoundManagerComponent() {
  auto ptr = new OpenALSoundManagerComponentImpl();
  ComObject<ISoundManagerComponent> impl(ptr);
  MGDFError error = ptr->Init();
  if (MGDF_OK != error) {
    return ComObject<ISoundManagerComponent>();
  }
  return impl;
}

OpenALSoundManagerComponentImpl::OpenALSoundManagerComponentImpl()
    : _enableAttenuation(false),
      _orientationForward(XMFLOAT3(0.0f, 0.0f, 1.0f)),
      _orientationUp(XMFLOAT3(0.0f, 1.0f, 0.0f)),
      _position(XMFLOAT3(0.0f, 0.0f, 0.0f)),
      _velocity(XMFLOAT3(0.0f, 0.0f, 0.0f)) {}

MGDFError OpenALSoundManagerComponentImpl::Init() {
  MGDFError error = OpenALSoundSystem::Init();
  if (MGDF_OK != error) {
    return error;
  }

  alDistanceModel(AL_NONE);

  return MGDF_OK;
}

OpenALSoundManagerComponentImpl::~OpenALSoundManagerComponentImpl() {
  for (auto sound : _sounds) {
    ComObject<IWString> name;
    sound->GetName(name.Assign());
    LOG("Sound '" << Resources::ToString(name) << "' still has live references",
        LOG_ERROR);
  }
  for (auto stream : _soundStreams) {
    ComObject<IWString> name;
    stream->GetName(name.Assign());
    LOG("SoundStream '" << Resources::ToString(name)
                        << "' still has live references",
        LOG_ERROR);
  }
  for (auto buffer : _sharedBuffers) {
    alDeleteBuffers(1, &buffer.first);
    delete buffer.second;
  }
}

void OpenALSoundManagerComponentImpl::Update() {
  alListener3f(AL_POSITION, _position.x, _position.y, _position.z);
  alListener3f(AL_VELOCITY, _velocity.x, _velocity.y, _velocity.z);

  float orientation[6];
  orientation[0] = _orientationForward.x;  // forward vector x value
  orientation[1] = _orientationForward.y;  // forward vector y value
  orientation[2] = _orientationForward.z;  // forward vector z value
  orientation[3] = _orientationUp.x;       // up vector x value
  orientation[4] = _orientationUp.y;       // up vector y value
  orientation[5] = _orientationUp.z;       // up vector z value
  alListenerfv(AL_ORIENTATION, orientation);

  INT32 deactivatedSoundsCount = 0;

  LOG("Updating sounds...", LOG_HIGH);
  for (auto sound : _sounds) {
    if (!sound->IsActive()) {
      deactivatedSoundsCount++;
    }

    float attenuation = 1;
    if (_enableAttenuation) {
      // work out the sounds attenuation due to distance

      SoundPosition sp;
      sound->GetPosition(&sp);
      XMVECTOR distanceVector = XMVectorSet(
          _position.x - sp.x, _position.y - sp.y, _position.z - sp.z, 1.0f);
      float distance = XMVectorGetX(XMVector3Length(distanceVector));

      if (distance <= sound->GetInnerRange()) {
        attenuation = 1;
      } else if (distance >= sound->GetOuterRange()) {
        attenuation = 0;
      } else {
        attenuation = 1 - ((distance - sound->GetInnerRange()) /
                           (sound->GetOuterRange() - sound->GetInnerRange()));
      }
    }
    sound->Update(attenuation);
  }

  if (deactivatedSoundsCount > 0) {
    PrioritizeSounds(deactivatedSoundsCount);
  }

  for (auto stream : _soundStreams) {
    stream->Update();
  }
}

SoundPosition *OpenALSoundManagerComponentImpl::GetListenerOrientationForward(
    SoundPosition *sp) const {
  memcpy_s(sp, sizeof(SoundPosition), &_orientationForward,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::GetListenerOrientationUp(
    SoundPosition *sp) const {
  memcpy_s(sp, sizeof(SoundPosition), &_orientationUp,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::GetListenerPosition(
    SoundPosition *sp) const {
  memcpy_s(sp, sizeof(SoundPosition), &_position, sizeof(DirectX::XMFLOAT3));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::GetListenerVelocity(
    SoundPosition *sp) const {
  memcpy_s(sp, sizeof(SoundPosition), &_velocity, sizeof(DirectX::XMFLOAT3));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::SetListenerOrientationForward(
    SoundPosition *sp) {
  memcpy_s(&_orientationForward, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(SoundPosition));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::SetListenerOrientationUp(
    SoundPosition *sp) {
  memcpy_s(&_orientationUp, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(SoundPosition));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::SetListenerPosition(
    SoundPosition *sp) {
  memcpy_s(&_position, sizeof(DirectX::XMFLOAT3), sp, sizeof(SoundPosition));
  return sp;
}

SoundPosition *OpenALSoundManagerComponentImpl::SetListenerVelocity(
    SoundPosition *sp) {
  memcpy_s(&_velocity, sizeof(DirectX::XMFLOAT3), sp, sizeof(SoundPosition));
  return sp;
}

bool OpenALSoundManagerComponentImpl::GetEnableAttenuation() const {
  return _enableAttenuation;
}

void OpenALSoundManagerComponentImpl::SetEnableAttenuation(
    bool enableAttenuation) {
  _enableAttenuation = enableAttenuation;
}

float OpenALSoundManagerComponentImpl::GetSoundVolume() const {
  return _soundVolume;
}

void OpenALSoundManagerComponentImpl::SetSoundVolume(float volume) {
  _soundVolume = volume;
  for (auto &sound : _sounds) {
    sound->SetGlobalVolume(_soundVolume);
  }
}

float OpenALSoundManagerComponentImpl::GetStreamVolume() const {
  return _streamVolume;
}

void OpenALSoundManagerComponentImpl::SetStreamVolume(float volume) {
  _streamVolume = volume;
  for (auto &stream : _soundStreams) {
    stream->SetGlobalVolume(_streamVolume);
  }
}

float OpenALSoundManagerComponentImpl::GetDopplerShiftFactor() const {
  return alGetFloat(AL_DOPPLER_FACTOR);
}

void OpenALSoundManagerComponentImpl::SetDopplerShiftFactor(
    float dopplerShiftFactor) {
  alDopplerFactor(dopplerShiftFactor);
}

float OpenALSoundManagerComponentImpl::GetSpeedOfSound() const {
  return alGetFloat(AL_SPEED_OF_SOUND);
}

void OpenALSoundManagerComponentImpl::SetSpeedOfSound(float speedOfSound) {
  alSpeedOfSound(speedOfSound);
}

HRESULT OpenALSoundManagerComponentImpl::CreateSoundStream(
    IFile *file, ISoundStream **stream) {
  if (!file) {
    LOG("The stream datasource cannot be null", LOG_ERROR);
    return MGDF_ERR_INVALID_FILE;
  }

  // try to deactivate a sound in order to free up a source for the new sound
  // however it may not be possible to deactivate any of the current sounds so
  // the new sound may have to be created as inactive.
  if (GetFreeSources() == 0) {
    LOG("Trying to free up audio source by deactivating low priority sound...",
        LOG_MEDIUM);
    DeactivateSound(INT_MAX);
  }

  // if we couldn't deactivate any sources then we cannot create the stream
  if (GetFreeSources() == 0) {
    return E_FAIL;
  } else {
    ComObject<VorbisStream> s;
    MGDFError error = VorbisStream::TryCreate(file, this, s);
    if (MGDF_OK != error) {
      return E_FAIL;
    }
    _soundStreams.insert(s);
    s.AddRawRef(stream);
    return S_OK;
  }
}

HRESULT OpenALSoundManagerComponentImpl::CreateSound(IFile *file,
                                                     INT32 priority,
                                                     ISound **sound) {
  if (!file) {
    LOG("The stream datasource cannot be null", LOG_ERROR);
    return MGDF_ERR_INVALID_FILE;
  }

  // try to deactivate a sound in order to free up a source for the new sound
  // however it may not be possible to deactivate any of the current sounds so
  // the new sound may have to be created as inactive.
  if (GetFreeSources() == 0) {
    LOG("Trying to free up audio source by deactivating low priority sound...",
        LOG_MEDIUM);
    DeactivateSound(priority);
  }

  ComObject<OpenALSound> s;
  MGDFError error = OpenALSound::TryCreate(file, this, priority, s);
  if (MGDF_OK != error) {
    return E_FAIL;
  }
  _sounds.insert(s);
  s.AddRawRef(sound);
  return S_OK;
}

void OpenALSoundManagerComponentImpl::GetPreferences(
    IPreferenceSet **preferences) {
  ComObject<PreferenceSetImpl> p(new PreferenceSetImpl());
  std::ostringstream ss;
  ss << _soundVolume;
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::SOUND_VOLUME, ss.str()));
  ss.clear();
  ss << _streamVolume;
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::MUSIC_VOLUME, ss.str()));
  p.AddRawRef(preferences);
}

void OpenALSoundManagerComponentImpl::DeactivateSound(INT32 priority) {
  // find all sounds with a priority equal or lower to the one to be created
  std::vector<OpenALSound *> sounds;
  for (auto sound : _sounds) {
    if (sound->GetPriority() <= priority) {
      sounds.push_back(sound);
    }
  }
  LOG("Deactivating " << sounds.size() << " sounds...", LOG_MEDIUM);
  if (sounds.size() > 0) {
    sort(sounds.begin(), sounds.end(), &OpenALSoundManagerComponentImpl::Sort);
    sounds[0]->Deactivate();
  }
}

// ensure as many samples are active as possible, with the highest precedence
// samples being activated first
void OpenALSoundManagerComponentImpl::PrioritizeSounds(
    INT32 deactivatedSoundsCount) {
  LOG("Prioritizing sounds...", LOG_MEDIUM);
  // copy the sounds into a local list so sorting won't mess up the external
  // ordering of the samples
  std::vector<OpenALSound *> sounds;
  for (const auto s : _sounds) {
    sounds.push_back(s);
  }
  sort(sounds.begin(), sounds.end(), &OpenALSoundManagerComponentImpl::Sort);

  // detect how many samples will need to be deactivated
  size_t freeSources = GetFreeSources();
  size_t requiringDeactivationCount = deactivatedSoundsCount - freeSources;
  if (requiringDeactivationCount < 0)
    requiringDeactivationCount = 0;  // we can activate all sounds.

  // deactivate requiringDeactivationCount of the lowest priority samples, and
  // reactivate the rest.
  for (auto s : sounds) {
    if (requiringDeactivationCount-- > 0) {
      if (s->IsActive()) {
        s->Deactivate();
      }
    } else {
      if (!s->IsActive()) {
        s->Reactivate();
      }
    }
  }
}

// sort sounds into the lowest->highest priority
bool OpenALSoundManagerComponentImpl::Sort(const OpenALSound *a,
                                           const OpenALSound *b) {
  _ASSERTE(a);
  _ASSERTE(b);

  if (a->GetPriority() < b->GetPriority()) {
    return true;
  } else if (a->GetAttenuatedVolume() < b->GetAttenuatedVolume()) {
    return true;
  } else if (!a->GetLooping() && b->GetLooping()) {
    return true;
  } else {
    return false;
  }
}

MGDFError OpenALSoundManagerComponentImpl::CreateSoundBuffer(IFile *dataSource,
                                                             ALuint *bufferId) {
  _ASSERTE(dataSource);

  LOG("Getting sound buffer...", LOG_MEDIUM);

  std::wstring dataSourceName = dataSource->GetLogicalPath();
  dataSourceName.append(L"/");
  dataSourceName.append(dataSource->GetName());

  // see if the buffer already exists in memory before trying to create it
  for (auto buffer : _sharedBuffers) {
    if (buffer.second->BufferSource == dataSourceName) {
      LOG("Sound buffer already loaded into memory - re-using", LOG_MEDIUM);
      ++buffer.second->References;
      *bufferId = buffer.first;
      return MGDF_OK;
    }
  }

  char *data;
  UINT32 truncSize;
  {
    ComObject<IFileReader> reader;
    if (FAILED(dataSource->Open(reader.Assign()))) {
      LOG("Buffer file could not be opened or is already open for reading",
          LOG_ERROR);
      // TODO replace with HRESULT
      return MGDF_ERR_FILE_IN_USE;
    }

    INT64 size = reader->GetSize();
    truncSize = size > UINT_MAX ? UINT_MAX : static_cast<UINT32>(size);
    data = new char[truncSize];
    reader->Read((void *)data, truncSize);
  }

  *bufferId = alutCreateBufferFromFileImage((ALvoid *)data, truncSize);
  delete[] data;

  // if the buffer loaded ok, add it to the list of loaded shared buffers
  if (*bufferId != ALUT_ERROR_AL_ERROR_ON_ENTRY &&
      *bufferId != ALUT_ERROR_ALC_ERROR_ON_ENTRY && *bufferId != AL_NONE) {
    LOG("Loaded shared sound buffer into memory", LOG_MEDIUM);
    SharedBuffer *sharedBuffer = new SharedBuffer();
    sharedBuffer->BufferSource = dataSourceName;
    sharedBuffer->References = 1;
    _sharedBuffers[*bufferId] = sharedBuffer;
    return MGDF_OK;
  }

  LOG("Error allocating sound buffer", LOG_ERROR);
  return MGDF_ERR_ERROR_ALLOCATING_BUFFER;
}

void OpenALSoundManagerComponentImpl::RemoveSoundBuffer(ALuint bufferId) {
  if (_sharedBuffers.find(bufferId) != _sharedBuffers.end()) {
    --_sharedBuffers[bufferId]->References;
    // if there are no more references to this buffer, remove it.
    if (_sharedBuffers[bufferId]->References == 0) {
      LOG("No more references to shared sound buffer - removing...",
          LOG_MEDIUM);
      alDeleteBuffers(1, &bufferId);
      delete _sharedBuffers[bufferId];
      _sharedBuffers.erase(bufferId);
    }
  }
}

void OpenALSoundManagerComponentImpl::RemoveSoundStream(ISoundStream *stream) {
  if (!stream) return;
  LOG("Removing sound stream", LOG_MEDIUM);
  _soundStreams.erase(static_cast<VorbisStream *>(stream));
}

void OpenALSoundManagerComponentImpl::RemoveSound(ISound *sound) {
  if (!sound) return;
  LOG("Removing sound", LOG_MEDIUM);
  _sounds.erase(static_cast<OpenALSound *>(sound));
}

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF