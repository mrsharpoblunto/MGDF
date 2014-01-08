#pragma once

#include <al.h>
#include <alc.h>
#include <boost/unordered_map.hpp>
#include <vector>
#include <string>

#include <MGDF/MGDF.hpp>
#include "../../common/MGDFListImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundSystem.hpp"

using namespace DirectX;

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

class OpenALSoundManagerComponentImpl: public ISoundManagerComponent
{
	friend class OpenALSound;
	friend class VorbisStream;
public:
	OpenALSoundManagerComponentImpl( IVirtualFileSystem *vfs );
	virtual ~OpenALSoundManagerComponentImpl();
	void Update() override;

	XMFLOAT3 *GetListenerPosition() override;
	XMFLOAT3 *GetListenerVelocity() override;
	XMFLOAT3 *GetListenerOrientationForward() override;
	XMFLOAT3 *GetListenerOrientationUp() override;

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
private:
	ALuint GetSoundBuffer( IFile *dataSource );
	void RemoveSoundBuffer( ALuint bufferId );

	void RemoveSoundStream( ISoundStream *stream );
	void RemoveSound( ISound *sound );

	void DeactivateSound( INT32 priority );
	void PrioritizeSounds( INT32 deactivatedSoundsCount );

	static bool Sort( OpenALSound *a, OpenALSound *b );

	XMFLOAT3 _position;
	XMFLOAT3 _velocity;
	XMFLOAT3 _orientationForward;
	XMFLOAT3 _orientationUp;
	ALCcontext *_context;
	float _soundVolume, _streamVolume;
	bool _enableAttenuation;
	boost::unordered_map<ALuint, SharedBuffer *> _sharedBuffers;
	std::vector<OpenALSound *> _sounds;
	std::vector<VorbisStream *> _soundStreams;
	IVirtualFileSystem *_vfs;
};

ISoundManagerComponent *CreateOpenALSoundManagerComponent( IVirtualFileSystem *vfs );

}
}
}
}