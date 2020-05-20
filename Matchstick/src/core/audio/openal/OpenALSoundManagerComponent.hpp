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
  INT32 References;
} SharedBuffer;

class OpenALSoundManagerComponentImpl : public OpenALSoundSystem,
                                        public ISoundManagerComponent {
  friend class OpenALSound;
  friend class VorbisStream;

 public:
  static ComObject<ISoundManagerComponent> CreateOpenALSoundManagerComponent();

  virtual ~OpenALSoundManagerComponentImpl();
  void Update() override final;

  SoundPosition *GetListenerPosition(
      SoundPosition *position) const override final;
  SoundPosition *GetListenerVelocity(
      SoundPosition *velocity) const override final;
  SoundPosition *GetListenerOrientationForward(
      SoundPosition *orientationForward) const override final;
  SoundPosition *GetListenerOrientationUp(
      SoundPosition *orientationUp) const override final;
  SoundPosition *SetListenerPosition(SoundPosition *position) override final;
  SoundPosition *SetListenerVelocity(SoundPosition *velocity) override final;
  SoundPosition *SetListenerOrientationForward(
      SoundPosition *orientationForward) override final;
  SoundPosition *SetListenerOrientationUp(
      SoundPosition *orientationUp) override final;

  float GetSoundVolume() const override final;
  void SetSoundVolume(float volume) override final;
  float GetStreamVolume() const override final;
  void SetStreamVolume(float volume) override final;

  bool GetEnableAttenuation() const override final;
  void SetEnableAttenuation(bool enableAttenuation) override final;
  float GetDopplerShiftFactor() const override final;
  void SetDopplerShiftFactor(float dopplerShiftFactor) override final;
  float GetSpeedOfSound() const override final;
  void SetSpeedOfSound(float speedOfSound) override final;

  HRESULT CreateSound(IFile *source, INT32 priority,
                      ISound **sound) override final;
  HRESULT CreateSoundStream(IFile *source,
                            ISoundStream **stream) override final;

  void RemoveSoundStream(ISoundStream *stream);
  void RemoveSound(ISound *sound);

  MGDFError CreateSoundBuffer(IFile *dataSource, ALuint *bufferId);
  void RemoveSoundBuffer(ALuint bufferId);

 private:
  OpenALSoundManagerComponentImpl();
  MGDFError Init() override final;

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