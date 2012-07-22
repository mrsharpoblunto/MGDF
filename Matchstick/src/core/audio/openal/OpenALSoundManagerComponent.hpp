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

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

typedef struct {
	std::wstring BufferSource;
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

MGDF_CORE_DLL ISoundManagerComponent *CreateOpenALSoundManagerComponent(HINSTANCE instance,IVirtualFileSystem *vfs,HWND window);

}}}}