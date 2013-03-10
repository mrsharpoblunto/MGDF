#pragma once

#include "al.h"
#include "alc.h"
#include <boost/unordered_map.hpp>
#include <vector>
#include <string>

#include <MGDF/MGDF.hpp>
#include "../../common/MGDFListImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundSystem.hpp"

using namespace DirectX;

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

typedef struct {
	std::wstring BufferSource;
	INT32 References;
} SharedBuffer;

class OpenALSoundManagerComponentImpl: public ISoundManagerComponent {
friend class OpenALSound;
friend class VorbisStream;
public:
	OpenALSoundManagerComponentImpl(IVirtualFileSystem *vfs);
	virtual ~OpenALSoundManagerComponentImpl();
	virtual void Update();

	virtual XMFLOAT3 *GetListenerPosition();
	virtual XMFLOAT3 *GetListenerVelocity();
	virtual XMFLOAT3 *GetListenerOrientationForward();
	virtual XMFLOAT3 *GetListenerOrientationUp();

	virtual float GetSoundVolume() const;
	virtual void SetSoundVolume(float volume);
	virtual float GetStreamVolume() const;
	virtual void SetStreamVolume(float volume);

	virtual bool GetEnableAttenuation() const;   
	virtual void SetEnableAttenuation(bool enableAttenuation);
	virtual float GetDopplerShiftFactor() const;
	virtual void SetDopplerShiftFactor(float dopplerShiftFactor);
	virtual float GetSpeedOfSound() const;
	virtual void SetSpeedOfSound(float speedOfSound);

	virtual ISound *CreateSound(IFile *source, INT32 priority);
	virtual ISoundStream *CreateSoundStream(IFile *source);
private:
	ALuint GetSoundBuffer(IFile *dataSource);
	void RemoveSoundBuffer(ALuint bufferId);

	void RemoveSoundStream(ISoundStream *stream);
	void RemoveSound(ISound *sound);

	void DeactivateSound(INT32 priority);
	void PrioritizeSounds(INT32 deactivatedSoundsCount);

	static bool Sort(OpenALSound *a,OpenALSound *b);

	XMFLOAT3 _position;
	XMFLOAT3 _velocity;
	XMFLOAT3 _orientationForward;
	XMFLOAT3 _orientationUp;
	ALCcontext *_context;
	float _soundVolume,_streamVolume;
	bool _enableAttenuation;
	boost::unordered_map<ALuint,SharedBuffer *> _sharedBuffers;
	std::vector<ISound *> _sounds;
	std::vector<ISoundStream *> _soundStreams;
	IVirtualFileSystem *_vfs;
};

ISoundManagerComponent *CreateOpenALSoundManagerComponent(IVirtualFileSystem *vfs);

}}}}