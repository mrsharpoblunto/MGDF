#pragma once

#include <AL/alut.h>
#include <al.h>
#include <alc.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <vector>

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

  void GetName(IWString **name) override final;
  SoundPosition *GetPosition(SoundPosition *position) const override final;
  SoundPosition *GetVelocity(SoundPosition *velocity) const override final;
  SoundPosition *SetPosition(SoundPosition *position) override final;
  SoundPosition *SetVelocity(SoundPosition *velocity) override final;
  float GetInnerRange() const override final;
  void SetInnerRange(float innerRange) override final;
  float GetOuterRange() const override final;
  void SetOuterRange(float outerRange) override final;
  bool GetSourceRelative() const override final;
  void SetSourceRelative(bool sourceRelative) override final;
  float GetVolume() const override final;
  void SetVolume(float volume) override final;
  float GetPitch() const override final;
  void SetPitch(float pitch) override final;
  void SetPriority(INT32 priority) override final;
  INT32 GetPriority() const override final;
  bool GetLooping() const override final;
  void SetLooping(bool looping) override final;
  void Stop() override final;
  void Pause() override final;
  void Play() override final;
  bool IsStopped() const override final;
  bool IsPaused() const override final;
  bool IsPlaying() const override final;
  bool IsActive() const override final;

  float GetAttenuatedVolume() const;
  void Reactivate();
  void Deactivate();
  void SetGlobalVolume(float globalVolume);
  void Update(float attenuationFactor);

 private:
  OpenALSound(OpenALSoundManagerComponentImpl *manager, INT32 priority);
  MGDFError Init(IFile *source);

  const wchar_t *_name;
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