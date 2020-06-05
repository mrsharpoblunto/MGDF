#pragma once

#include <al.h>
#include <alc.h>

#include <MGDF/MGDF.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../common/MGDFListImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundSystem.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

typedef struct {
  std::wstring BufferSource;
  INT32 References = 0;
} SharedBuffer;

class OpenALSoundManagerComponentImpl : public OpenALSoundSystem,
                                        public ISoundManagerComponent {
  friend class OpenALSound;
  friend class VorbisStream;

 public:
  static bool CreateOpenALSoundManagerComponent(
      ComObject<ISoundManagerComponent> &comp);

  virtual ~OpenALSoundManagerComponentImpl();
  OpenALSoundManagerComponentImpl();
  void Update() final;

  SoundPosition *GetListenerPosition(SoundPosition *position) const final;
  SoundPosition *GetListenerVelocity(SoundPosition *velocity) const final;
  SoundPosition *GetListenerOrientationForward(
      SoundPosition *orientationForward) const final;
  SoundPosition *GetListenerOrientationUp(
      SoundPosition *orientationUp) const final;
  SoundPosition *SetListenerPosition(SoundPosition *position) final;
  SoundPosition *SetListenerVelocity(SoundPosition *velocity) final;
  SoundPosition *SetListenerOrientationForward(
      SoundPosition *orientationForward) final;
  SoundPosition *SetListenerOrientationUp(SoundPosition *orientationUp) final;

  float GetSoundVolume() const final;
  void SetSoundVolume(float volume) final;
  float GetStreamVolume() const final;
  void SetStreamVolume(float volume) final;

  bool GetEnableAttenuation() const final;
  void SetEnableAttenuation(bool enableAttenuation) final;
  float GetDopplerShiftFactor() const final;
  void SetDopplerShiftFactor(float dopplerShiftFactor) final;
  float GetSpeedOfSound() const final;
  void SetSpeedOfSound(float speedOfSound) final;

  HRESULT CreateSound(IFile *source, INT32 priority, ISound **sound) final;
  HRESULT CreateSoundStream(IFile *source, ISoundStream **stream) final;
  void GetPreferences(IPreferenceSet **preferences) final;
  void RemoveSoundStream(ISoundStream *stream);
  void RemoveSound(ISound *sound);

  MGDFError CreateSoundBuffer(IFile *dataSource, ALuint *bufferId);
  void RemoveSoundBuffer(ALuint bufferId);

 private:
  MGDFError Init() final;

  void DeactivateSound(INT32 priority);
  void PrioritizeSounds(INT32 deactivatedSoundsCount);

  static bool Sort(const OpenALSound *a, const OpenALSound *b);

  DirectX::XMFLOAT3 _position;
  DirectX::XMFLOAT3 _velocity;
  DirectX::XMFLOAT3 _orientationForward;
  DirectX::XMFLOAT3 _orientationUp;

  float _soundVolume, _streamVolume;
  bool _enableAttenuation;
  std::unordered_map<ALuint, SharedBuffer *> _sharedBuffers;
  std::set<OpenALSound *> _sounds;
  std::set<VorbisStream *> _soundStreams;
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF