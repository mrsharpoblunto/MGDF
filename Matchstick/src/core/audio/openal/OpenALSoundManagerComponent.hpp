#pragma once

#include <al.h>
#include <alc.h>
#include <unordered_map>
#include <vector>
#include <string>

#include <MGDF/MGDF.hpp>
#include "../../common/MGDFListImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundSystem.hpp"

namespace MGDF
{
namespace core
{
namespace audio
{
namespace openal_audio
{

typedef struct {
	std::wstring BufferSource;
	INT32 References;
} SharedBuffer;

class OpenALSoundManagerComponentImpl: public OpenALSoundSystem, public ISoundManagerComponent
{
	friend class OpenALSound;
	friend class VorbisStream;
public:
	static ISoundManagerComponent *CreateOpenALSoundManagerComponent( IVirtualFileSystem *vfs );

	virtual ~OpenALSoundManagerComponentImpl();
	void Update() override;

	DirectX::XMFLOAT3 *GetListenerPosition() override;
	DirectX::XMFLOAT3 *GetListenerVelocity() override;
	DirectX::XMFLOAT3 *GetListenerOrientationForward() override;
	DirectX::XMFLOAT3 *GetListenerOrientationUp() override;

	float GetSoundVolume() const override;
	void SetSoundVolume( float volume ) override;
	float GetStreamVolume() const override;
	void SetStreamVolume( float volume ) override;

	bool GetEnableAttenuation() const override;
	void SetEnableAttenuation( bool enableAttenuation ) override;
	float GetDopplerShiftFactor() const override;
	void SetDopplerShiftFactor( float dopplerShiftFactor ) override;
	float GetSpeedOfSound() const override;
	void SetSpeedOfSound( float speedOfSound ) override;

	MGDFError CreateSound( IFile *source, INT32 priority, ISound **sound ) override;
	MGDFError CreateSoundStream( IFile *source, ISoundStream **stream ) override;
	
	void RemoveSoundStream( ISoundStream *stream );
	void RemoveSound( ISound *sound );

	MGDFError CreateSoundBuffer( IFile *dataSource, ALuint *bufferId );
	void RemoveSoundBuffer( ALuint bufferId );

private:
	OpenALSoundManagerComponentImpl( IVirtualFileSystem *vfs );
	MGDFError Init() override;

	void DeactivateSound( INT32 priority );
	void PrioritizeSounds( INT32 deactivatedSoundsCount );

	static bool Sort( OpenALSound *a, OpenALSound *b );

	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _velocity;
	DirectX::XMFLOAT3 _orientationForward;
	DirectX::XMFLOAT3 _orientationUp;

	float _soundVolume, _streamVolume;
	bool _enableAttenuation;
	std::unordered_map<ALuint, SharedBuffer *> _sharedBuffers;
	std::vector<OpenALSound *> _sounds;
	std::vector<VorbisStream *> _soundStreams;
	IVirtualFileSystem *_vfs;
};

}
}
}
}