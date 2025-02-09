#pragma once

#include <AL/al.h>
#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "OpenALSoundManagerComponentImpl.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

class OpenALSound : public ComBase<IMGDFSound> {
 public:
  virtual ~OpenALSound();
  OpenALSound(OpenALSoundManagerComponentImpl *manager, INT32 priority);
  static HRESULT TryCreate(ComObject<IMGDFReadOnlyFile> source,
                           OpenALSoundManagerComponentImpl *manager,
                           INT32 priority, ComObject<OpenALSound> &sound);

  HRESULT __stdcall GetName(wchar_t *name, UINT64 *length) final;
  MGDFSoundPosition *__stdcall GetPosition(MGDFSoundPosition *position) final;
  MGDFSoundPosition *__stdcall GetVelocity(MGDFSoundPosition *velocity) final;
  MGDFSoundPosition *__stdcall SetPosition(MGDFSoundPosition *position) final;
  MGDFSoundPosition *__stdcall SetVelocity(MGDFSoundPosition *velocity) final;
  float __stdcall GetInnerRange() final;
  void __stdcall SetInnerRange(float innerRange) final;
  float __stdcall GetOuterRange() final;
  void __stdcall SetOuterRange(float outerRange) final;
  BOOL __stdcall GetSourceRelative() final;
  void __stdcall SetSourceRelative(BOOL sourceRelative) final;
  float __stdcall GetVolume() final;
  void __stdcall SetVolume(float volume) final;
  float __stdcall GetPitch() final;
  void __stdcall SetPitch(float pitch) final;
  void __stdcall SetPriority(INT32 priority) final;
  INT32 __stdcall GetPriority() final;
  BOOL __stdcall GetLooping() final;
  void __stdcall SetLooping(BOOL looping) final;
  void __stdcall Stop() final;
  void __stdcall Pause() final;
  void __stdcall Play() final;
  BOOL __stdcall IsStopped() final;
  BOOL __stdcall IsPaused() final;
  BOOL __stdcall IsPlaying() final;
  BOOL __stdcall IsActive() final;

  float GetAttenuatedVolume() const;
  void Reactivate();
  void Deactivate();
  void SetGlobalVolume(float globalVolume);
  void Update(float attenuationFactor);

 private:
  HRESULT Init(ComObject<IMGDFReadOnlyFile> source);

  std::wstring _name;
  ComObject<OpenALSoundManagerComponentImpl> _soundManager;
  ALuint _sourceId, _bufferId;
  float _innerRange, _outerRange, _volume, _globalVolume, _attenuationFactor,
      _pitch;
  BOOL _isActive, _isSourceRelative, _isLooping, _wasPlaying, _startPlaying;
  INT32 _priority;
  DirectX::XMFLOAT3 _position;
  DirectX::XMFLOAT3 _velocity;
};

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF