#pragma once

#include <AL/alut.h>
#include <al.h>
#include <alc.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <vector>

#include "../../common/MGDFStringImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

class OpenALSound : public ComBase<ISound> {
 public:
  virtual ~OpenALSound();
  static MGDFError TryCreate(IFile *source,
                             OpenALSoundManagerComponentImpl *manager,
                             INT32 priority, ComObject<OpenALSound> &sound);

  HRESULT GetName(wchar_t *name, size_t *length) final;
  SoundPosition *GetPosition(SoundPosition *position) const final;
  SoundPosition *GetVelocity(SoundPosition *velocity) const final;
  SoundPosition *SetPosition(SoundPosition *position) final;
  SoundPosition *SetVelocity(SoundPosition *velocity) final;
  float GetInnerRange() const final;
  void SetInnerRange(float innerRange) final;
  float GetOuterRange() const final;
  void SetOuterRange(float outerRange) final;
  bool GetSourceRelative() const final;
  void SetSourceRelative(bool sourceRelative) final;
  float GetVolume() const final;
  void SetVolume(float volume) final;
  float GetPitch() const final;
  void SetPitch(float pitch) final;
  void SetPriority(INT32 priority) final;
  INT32 GetPriority() const final;
  bool GetLooping() const final;
  void SetLooping(bool looping) final;
  void Stop() final;
  void Pause() final;
  void Play() final;
  bool IsStopped() const final;
  bool IsPaused() const final;
  bool IsPlaying() const final;
  bool IsActive() const final;

  float GetAttenuatedVolume() const;
  void Reactivate();
  void Deactivate();
  void SetGlobalVolume(float globalVolume);
  void Update(float attenuationFactor);

 private:
  OpenALSound(OpenALSoundManagerComponentImpl *manager, INT32 priority);
  MGDFError Init(IFile *source);

  std::wstring _name;
  OpenALSoundManagerComponentImpl *_soundManager;
  ALuint _sourceId, _bufferId;
  float _innerRange, _outerRange, _volume, _globalVolume, _attenuationFactor,
      _pitch;
  bool _isActive, _isSourceRelative, _isLooping, _wasPlaying, _startPlaying;
  INT32 _priority;
  DirectX::XMFLOAT3 _position;
  DirectX::XMFLOAT3 _velocity;
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF