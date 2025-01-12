
#include "StdAfx.h"

#include "OpenALSoundManagerComponentImpl.hpp"

#include <AL/alut.h>
#include <al.h>

#include <algorithm>

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFPreferenceConstants.hpp"
#include "../../common/MGDFPreferenceSet.hpp"
#include "../../common/MGDFResources.hpp"
#include "OpenALSound.hpp"
#include "OpenALSoundSystem.hpp"
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

OpenALSoundManagerComponentImpl::OpenALSoundManagerComponentImpl(
    const ComObject<IMGDFReadOnlyVirtualFileSystem> &vfs)
    : _enableAttenuation(false),
      _vfs(vfs),
      _soundVolume(0.0f),
      _streamVolume(0.0f),
      _orientationForward(XMFLOAT3(0.0f, 0.0f, 1.0f)),
      _orientationUp(XMFLOAT3(0.0f, 1.0f, 0.0f)),
      _position(XMFLOAT3(0.0f, 0.0f, 0.0f)),
      _velocity(XMFLOAT3(0.0f, 0.0f, 0.0f)) {}

HRESULT OpenALSoundManagerComponentImpl::Init() {
  const auto result = OpenALSoundSystem::Init();
  if (FAILED(result)) {
    return result;
  }

  alDistanceModel(AL_NONE);

  return S_OK;
}

OpenALSoundManagerComponentImpl::~OpenALSoundManagerComponentImpl() {
  for (auto sound : _sounds) {
    std::wstring name = ComString<&IMGDFSound::GetName>(sound);
    LOG("Sound '" << Resources::ToString(name) << "' still has live references",
        MGDF_LOG_ERROR);
  }
  for (auto stream : _soundStreams) {
    std::wstring name = ComString<&IMGDFSoundStream::GetName>(stream);
    LOG("SoundStream '" << Resources::ToString(name)
                        << "' still has live references",
        MGDF_LOG_ERROR);
  }
  for (auto &buffer : _sharedBuffers) {
    alDeleteBuffers(1, &buffer.first);
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

  LOG("Updating sounds...", MGDF_LOG_HIGH);
  for (auto sound : _sounds) {
    if (!sound->IsActive()) {
      deactivatedSoundsCount++;
    }

    float attenuation = 1;
    if (_enableAttenuation) {
      // work out the sounds attenuation due to distance

      MGDFSoundPosition sp;
      sound->GetPosition(&sp);
      const XMVECTOR distanceVector = XMVectorSet(
          _position.x - sp.x, _position.y - sp.y, _position.z - sp.z, 1.0f);
      const float distance = XMVectorGetX(XMVector3Length(distanceVector));

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

MGDFSoundPosition *
OpenALSoundManagerComponentImpl::GetListenerOrientationForward(
    MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_orientationForward,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::GetListenerOrientationUp(
    MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_orientationUp,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::GetListenerPosition(
    MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_position,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::GetListenerVelocity(
    MGDFSoundPosition *sp) {
  memcpy_s(sp, sizeof(MGDFSoundPosition), &_velocity,
           sizeof(DirectX::XMFLOAT3));
  return sp;
}

MGDFSoundPosition *
OpenALSoundManagerComponentImpl::SetListenerOrientationForward(
    MGDFSoundPosition *sp) {
  memcpy_s(&_orientationForward, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(MGDFSoundPosition));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::SetListenerOrientationUp(
    MGDFSoundPosition *sp) {
  memcpy_s(&_orientationUp, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(MGDFSoundPosition));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::SetListenerPosition(
    MGDFSoundPosition *sp) {
  memcpy_s(&_position, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(MGDFSoundPosition));
  return sp;
}

MGDFSoundPosition *OpenALSoundManagerComponentImpl::SetListenerVelocity(
    MGDFSoundPosition *sp) {
  memcpy_s(&_velocity, sizeof(DirectX::XMFLOAT3), sp,
           sizeof(MGDFSoundPosition));
  return sp;
}

BOOL OpenALSoundManagerComponentImpl::GetEnableAttenuation() {
  return _enableAttenuation;
}

void OpenALSoundManagerComponentImpl::SetEnableAttenuation(
    BOOL enableAttenuation) {
  _enableAttenuation = enableAttenuation;
}

float OpenALSoundManagerComponentImpl::GetSoundVolume() { return _soundVolume; }

void OpenALSoundManagerComponentImpl::SetSoundVolume(float volume) {
  _soundVolume = volume;
  for (auto &sound : _sounds) {
    sound->SetGlobalVolume(_soundVolume);
  }
}

float OpenALSoundManagerComponentImpl::GetStreamVolume() {
  return _streamVolume;
}

void OpenALSoundManagerComponentImpl::SetStreamVolume(float volume) {
  _streamVolume = volume;
  for (auto &stream : _soundStreams) {
    stream->SetGlobalVolume(_streamVolume);
  }
}

float OpenALSoundManagerComponentImpl::GetDopplerShiftFactor() {
  return alGetFloat(AL_DOPPLER_FACTOR);
}

void OpenALSoundManagerComponentImpl::SetDopplerShiftFactor(
    float dopplerShiftFactor) {
  alDopplerFactor(dopplerShiftFactor);
}

float OpenALSoundManagerComponentImpl::GetSpeedOfSound() {
  return alGetFloat(AL_SPEED_OF_SOUND);
}

void OpenALSoundManagerComponentImpl::SetSpeedOfSound(float speedOfSound) {
  alSpeedOfSound(speedOfSound);
}

HRESULT OpenALSoundManagerComponentImpl::CreateSoundStream(
    IMGDFReadOnlyFile *file, IMGDFSoundStream **stream) {
  if (!file) {
    LOG("The stream datasource cannot be null", MGDF_LOG_ERROR);
    return E_INVALIDARG;
  }

  // try to deactivate a sound in order to free up a source for the new sound
  // however it may not be possible to deactivate any of the current sounds so
  // the new sound may have to be created as inactive.
  if (GetFreeSources() == 0) {
    LOG("Trying to free up audio source by deactivating low priority sound...",
        MGDF_LOG_MEDIUM);
    DeactivateSound(INT_MAX);
  }

  // if we couldn't deactivate any sources then we cannot create the stream
  if (GetFreeSources() == 0) {
    return E_FAIL;
  } else {
    ComObject<VorbisStream> s;
    const auto result = VorbisStream::TryCreate(file, this, s);
    if (SUCCEEDED(result)) {
      _soundStreams.insert(s);
      s.AddRawRef(stream);
    }
    return result;
  }
}

HRESULT OpenALSoundManagerComponentImpl::CreateSound(IMGDFReadOnlyFile *file,
                                                     INT32 priority,
                                                     IMGDFSound **sound) {
  if (!file) {
    LOG("The stream datasource cannot be null", MGDF_LOG_ERROR);
    return E_INVALIDARG;
  }

  // try to deactivate a sound in order to free up a source for the new sound
  // however it may not be possible to deactivate any of the current sounds so
  // the new sound may have to be created as inactive.
  if (GetFreeSources() == 0) {
    LOG("Trying to free up audio source by deactivating low priority sound...",
        MGDF_LOG_MEDIUM);
    DeactivateSound(priority);
  }

  ComObject<OpenALSound> s;
  const auto result = OpenALSound::TryCreate(file, this, priority, s);
  if (SUCCEEDED(result)) {
    _sounds.insert(s);
    s.AddRawRef(sound);
  }
  return result;
}

void OpenALSoundManagerComponentImpl::GetPreferences(
    IMGDFPreferenceSet **preferences) {
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
  LOG("Deactivating " << sounds.size() << " sounds...", MGDF_LOG_MEDIUM);
  if (sounds.size() > 0) {
    sort(sounds.begin(), sounds.end(), &OpenALSoundManagerComponentImpl::Sort);
    sounds[0]->Deactivate();
  }
}

// ensure as many samples are active as possible, with the highest precedence
// samples being activated first
void OpenALSoundManagerComponentImpl::PrioritizeSounds(
    INT32 deactivatedSoundsCount) {
  LOG("Prioritizing sounds...", MGDF_LOG_MEDIUM);
  // copy the sounds into a local list so sorting won't mess up the external
  // ordering of the samples
  std::vector<OpenALSound *> sounds;
  for (const auto s : _sounds) {
    sounds.push_back(s);
  }
  sort(sounds.begin(), sounds.end(), &OpenALSoundManagerComponentImpl::Sort);

  // detect how many samples will need to be deactivated
  const size_t freeSources = GetFreeSources();
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
bool OpenALSoundManagerComponentImpl::Sort(OpenALSound *a, OpenALSound *b) {
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

HRESULT OpenALSoundManagerComponentImpl::CreateSoundBuffer(
    IMGDFReadOnlyFile *dataSource, ALuint *bufferId) {
  _ASSERTE(dataSource);

  LOG("Getting sound buffer...", MGDF_LOG_MEDIUM);

  std::wstring dataSourceName =
      ComString<&IMGDFReadOnlyVirtualFileSystem::GetLogicalPath>(_vfs,
                                                                 dataSource);

  // see if the buffer already exists in memory before trying to create it
  for (auto &buffer : _sharedBuffers) {
    if (buffer.second->BufferSource == dataSourceName) {
      LOG("Sound buffer already loaded into memory - re-using",
          MGDF_LOG_MEDIUM);
      ++buffer.second->References;
      *bufferId = buffer.first;
      return S_OK;
    }
  }

  {
    std::vector<char> data;
    ComObject<IMGDFFileReader> reader;
    if (FAILED(dataSource->Open(reader.Assign()))) {
      LOG("Buffer file could not be opened or is already open for reading",
          MGDF_LOG_ERROR);
      return E_ACCESSDENIED;
    }

    const INT64 size = reader->GetSize();
    const UINT32 truncSize =
        size > UINT_MAX ? UINT_MAX : static_cast<UINT32>(size);
    data.resize(truncSize);
    reader->Read(data.data(), truncSize);
    *bufferId = alutCreateBufferFromFileImage((ALvoid *)data.data(), truncSize);
  }

  // if the buffer loaded ok, add it to the list of loaded shared buffers
  if (*bufferId != ALUT_ERROR_AL_ERROR_ON_ENTRY &&
      *bufferId != ALUT_ERROR_ALC_ERROR_ON_ENTRY && *bufferId != AL_NONE) {
    LOG("Loaded shared sound buffer into memory", MGDF_LOG_MEDIUM);
    auto sharedBuffer = std::make_unique<SharedBuffer>();
    sharedBuffer->BufferSource = dataSourceName;
    sharedBuffer->References = 1;
    _sharedBuffers.insert(std::make_pair(*bufferId, std::move(sharedBuffer)));
    return S_OK;
  }

  LOG("Error allocating sound buffer", MGDF_LOG_ERROR);
  return E_FAIL;
}

void OpenALSoundManagerComponentImpl::RemoveSoundBuffer(ALuint bufferId) {
  if (_sharedBuffers.find(bufferId) != _sharedBuffers.end()) {
    --_sharedBuffers[bufferId]->References;
    // if there are no more references to this buffer, remove it.
    if (_sharedBuffers[bufferId]->References == 0) {
      LOG("No more references to shared sound buffer - removing...",
          MGDF_LOG_MEDIUM);
      alDeleteBuffers(1, &bufferId);
      _sharedBuffers.erase(bufferId);
    }
  }
}

void OpenALSoundManagerComponentImpl::RemoveSoundStream(
    IMGDFSoundStream *stream) {
  if (!stream) return;
  LOG("Removing sound stream", MGDF_LOG_MEDIUM);
  _soundStreams.erase(static_cast<VorbisStream *>(stream));
}

void OpenALSoundManagerComponentImpl::RemoveSound(IMGDFSound *sound) {
  if (!sound) return;
  LOG("Removing sound", MGDF_LOG_MEDIUM);
  _sounds.erase(static_cast<OpenALSound *>(sound));
}

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF
