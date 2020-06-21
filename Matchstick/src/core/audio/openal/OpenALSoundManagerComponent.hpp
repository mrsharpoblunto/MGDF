#pragma once

#include <al.h>
#include <alc.h>

#include <MGDF/MGDF.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <DirectXMath.h>

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

  MGDFSoundPosition * __stdcall GetListenerPosition(MGDFSoundPosition *position) final;
  MGDFSoundPosition * __stdcall GetListenerVelocity(MGDFSoundPosition *velocity) final;
  MGDFSoundPosition * __stdcall GetListenerOrientationForward(
      MGDFSoundPosition *orientationForward) final;
  MGDFSoundPosition * __stdcall GetListenerOrientationUp(
      MGDFSoundPosition *orientationUp) final;
  MGDFSoundPosition * __stdcall SetListenerPosition(MGDFSoundPosition *position) final;
  MGDFSoundPosition * __stdcall SetListenerVelocity(MGDFSoundPosition *velocity) final;
  MGDFSoundPosition * __stdcall SetListenerOrientationForward(
      MGDFSoundPosition *orientationForward) final;
  MGDFSoundPosition * __stdcall SetListenerOrientationUp(MGDFSoundPosition *orientationUp) final;

  float __stdcall GetSoundVolume() final;
  void __stdcall SetSoundVolume(float volume) final;
  float __stdcall GetStreamVolume() final;
  void __stdcall SetStreamVolume(float volume) final;

  BOOL __stdcall GetEnableAttenuation() final;
  void __stdcall SetEnableAttenuation(BOOL enableAttenuation) final;
  float __stdcall GetDopplerShiftFactor() final;
  void __stdcall SetDopplerShiftFactor(float dopplerShiftFactor) final;
  float __stdcall GetSpeedOfSound() final;
  void __stdcall SetSpeedOfSound(float speedOfSound) final;

  HRESULT __stdcall CreateSound(IMGDFFile *source, INT32 priority, IMGDFSound **sound) final;
  HRESULT __stdcall CreateSoundStream(IMGDFFile *source, IMGDFSoundStream **stream) final;
  void __stdcall GetPreferences(IMGDFPreferenceSet **preferences) final;

  void RemoveSoundStream(IMGDFSoundStream *stream);
  void RemoveSound(IMGDFSound *sound);
  HRESULT CreateSoundBuffer(IMGDFFile *dataSource, ALuint *bufferId);
  void RemoveSoundBuffer(ALuint bufferId);

 private:
  HRESULT Init() final;

  void DeactivateSound(INT32 priority);
  void PrioritizeSounds(INT32 deactivatedSoundsCount);

  static bool Sort(OpenALSound *a, OpenALSound *b);

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