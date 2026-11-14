#pragma once
#include <chrono>
#include <memory>

#include "../MGDFSoundManagerComponent.hpp"
namespace MGDF::core::audio::openal_audio {
struct AudioOptions {
  bool Loopback = false;
  UINT32 SampleRate = 48000;
  UINT32 Sources = 512;
  bool Hrtf = false;
};
class OpenALSoundManagerComponentImpl : public ISoundManagerComponent {
 public:
  explicit OpenALSoundManagerComponentImpl(AudioOptions options = {});
  ~OpenALSoundManagerComponentImpl();
  HRESULT Init();
  void Stop() final;
  MGDFBusId __stdcall GetBus(MGDFBusKind) final;
  HRESULT __stdcall CreateBus(const MGDFBusDesc *, MGDFBusId *) final;
  HRESULT __stdcall CreateSoundSamples(IMGDFReadOnlyFile *,
                                       const MGDFSoundSamplesDesc *,
                                       IMGDFSoundSamples **) final;
  HRESULT __stdcall CreatePcmStream(const MGDFSoundPcmDesc *,
                                    IMGDFSoundPcmStream **) final;
  HRESULT __stdcall CreateCommandList(IMGDFSoundCommandList **) final;
  void __stdcall Submit(IMGDFSoundCommandList *) final;
  double __stdcall GetAudioTime() final;
  UINT32 __stdcall GetSampleRate() final;
  MGDFVoiceStatus __stdcall GetVoiceStatus(MGDFVoiceId) final;
  void __stdcall GetStats(MGDFSoundStats *) final;
  void __stdcall SetSpeedOfSound(float) final;
  void __stdcall SetDopplerFactor(float) final;
  void __stdcall GetPreferences(IMGDFPreferenceSet **) final;
  // Loopback uses the caller as the audio thread.
  void Update();
  void Render(float *stereo, UINT32 frames);
  bool WaitForLoads(std::chrono::milliseconds timeout);

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
}  // namespace MGDF::core::audio::openal_audio
