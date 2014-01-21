#pragma once

#include <al.h>
#include <alc.h>
#include <AL/alut.h>

#include <vector>

#include <MGDF/MGDF.hpp>
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundManagerComponent.hpp"

namespace MGDF
{
namespace core
{
namespace audio
{
namespace openal_audio
{

class OpenALSound: public ISound
{
public:
	virtual ~OpenALSound();
	static MGDFError TryCreate( IFile *source, OpenALSoundManagerComponentImpl *manager, INT32 priority, OpenALSound **sound );

	const wchar_t *GetName() const override final;
	DirectX::XMFLOAT3 *GetPosition() override final;
	DirectX::XMFLOAT3 *GetVelocity() override final;
	float GetInnerRange() const override final;
	void SetInnerRange( float innerRange ) override final;
	float GetOuterRange() const override final;
	void SetOuterRange( float outerRange ) override final;
	bool GetSourceRelative() const override final;
	void SetSourceRelative( bool sourceRelative ) override final;
	float GetVolume() const override final;
	void SetVolume( float volume ) override final;
	float GetPitch() const override final;
	void SetPitch( float pitch ) override final;
	void SetPriority( INT32 priority ) override final;
	INT32 GetPriority() const override final;
	bool GetLooping() const override final;
	void SetLooping( bool looping ) override final;
	void Stop() override final;
	void Pause() override final;
	void Play() override final;
	bool IsStopped() const override final;
	bool IsPaused() const override final;
	bool IsPlaying() const override final;
	bool IsActive() const override final;

	HRESULT QueryInterface( REFIID riid, void **ppvObject ) override final;
	ULONG AddRef() override final;
	ULONG Release() override final;

	float GetAttenuatedVolume() const;
	void Reactivate();
	void Deactivate();
	void SetGlobalVolume( float globalVolume );
	void Update( float attenuationFactor );

private:
	OpenALSound( OpenALSoundManagerComponentImpl *manager, INT32 priority );
	MGDFError Init( IFile *source );

	ULONG _references;
	const wchar_t *_name;
	OpenALSoundManagerComponentImpl *_soundManager;
	ALuint _sourceId, _bufferId;
	float _innerRange, _outerRange, _volume, _globalVolume, _attenuationFactor, _pitch;
	bool _isActive, _isSourceRelative, _isLooping, _wasPlaying, _startPlaying;
	INT32 _priority;
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _velocity;
};

}
}
}
}