#pragma once

#include "al.h"
#include "alc.h"
#include <hash_map>
#include <vector>
#include <string>

#include <MGDF/MGDF.hpp>
#include "../../common/MGDFListImpl.hpp"
#include "../MGDFSoundManagerComponent.hpp"
#include "OpenALSoundSystem.hpp"

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

typedef struct {
	std::string BufferSource;
	int References;
} SharedBuffer;

class OpenALSoundManagerComponentImpl: public DisposeImpl<ISoundManagerComponent> {
friend class OpenALSound;
friend class VorbisStream;
public:
	OpenALSoundManagerComponentImpl(HINSTANCE instance,IVirtualFileSystem *vfs,HWND window);
	virtual ~OpenALSoundManagerComponentImpl();
	virtual void Update();
	virtual void Dispose();

	virtual Vector *GetListenerPosition() const;
	virtual Vector *GetListenerVelocity() const;
	virtual Vector *GetListenerOrientationForward() const;
	virtual Vector *GetListenerOrientationUp() const;

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

	virtual ISound *CreateSound(IFile *source, int priority);
	virtual void RemoveSound(ISound *sound);

	virtual ISoundStream *CreateSoundStream(IFile *source);
	virtual void RemoveSoundStream(ISoundStream *stream);
private:
	ALuint GetSoundBuffer(IFile *dataSource);
	void RemoveSoundBuffer(ALuint bufferId);

	void DoRemoveSoundStream(ISoundStream *stream);
	void DoRemoveSound(ISound *sound);

	void DeactivateSound(int priority);
	void PrioritizeSounds(int deactivatedSoundsCount);

	static bool Sort(OpenALSound *a,OpenALSound *b);

	Vector *_position;
	Vector *_velocity;
	Vector *_orientationForward;
	Vector *_orientationUp;
	ALCcontext *_context;
	float _soundVolume,_streamVolume;
	bool _enableAttenuation;
	stdext::hash_map<ALuint,SharedBuffer *> _sharedBuffers;
	std::vector<ISound *> _sounds;
	std::vector<ISoundStream *> _soundStreams;
	IVirtualFileSystem *_vfs;
};

MGDF_CORE_DLL ISoundManagerComponent *CreateOpenALSoundManagerComponent(HINSTANCE instance,IVirtualFileSystem *vfs,HWND window);

}}}}