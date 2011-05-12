#include "StdAfx.h"

#include "../../common/MGDFExceptions.hpp"
#include "../../common/MGDFLoggerImpl.hpp"
#include "OpenALSoundSystem.hpp"
#include "OpenALSound.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#pragma warning(disable:4345) //disable irrelevant warning about initializing POD types via new() syntax.

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

	OpenALSound::OpenALSound(IFile *source,OpenALSoundManagerComponentImpl *manager,int priority)
	{
		_soundManager = manager;
		_bufferId = _soundManager->GetSoundBuffer(source);
		if (_bufferId == ALUT_ERROR_AL_ERROR_ON_ENTRY || _bufferId == ALUT_ERROR_ALC_ERROR_ON_ENTRY)
		{
			_soundManager->GetComponentErrorHandler()->SetLastError(THIS_NAME,MGDF_ERR_ERROR_ALLOCATING_BUFFER,"Error allocating sound buffer");
			throw MGDFException("Error allocating sound buffer");
		}

		_name = source->GetName();
		_priority=priority;

		_innerRange=0;
		_outerRange=1;
		_volume=1;
		_globalVolume=manager->GetSoundVolume();
		_attenuationFactor=1;
		_pitch = 1;

		_isLooping = false;
		_isSourceRelative=true;
		_wasPlaying = false;
		_startPlaying = false;

		_position=new Vector();
		_position->X = 0;
		_position->Y = 0;
		_position->Z = 0;

		_velocity=new Vector();
		_velocity->X = 0;
		_velocity->Y = 0;
		_velocity->Z = 0;

		Reactivate();
	}

	OpenALSound::~OpenALSound()
	{
		delete _position;
		delete _velocity;

		_soundManager->DoRemoveSound(this);
		_soundManager->RemoveSoundBuffer(_bufferId);
		Deactivate();
	}

	void OpenALSound::Dispose()
	{
		delete this;
	}

	void OpenALSound::Reactivate()
	{
		_isActive = OpenALSoundSystem::InstancePtr()->AcquireSource(&_sourceId);

		if (_isActive) {
			alSourcei(_sourceId, AL_BUFFER,_bufferId);
			if (alGetError() != AL_NO_ERROR) {
				GetLoggerImpl()->Add(THIS_NAME,"Unable to allocate buffer to audio source",LOG_ERROR);
				Deactivate();
			}
			else {
				SetSourceRelative(_isSourceRelative);
				SetVolume(_volume);
				SetPitch(_pitch);
				SetLooping(_isLooping);
				if (_isLooping && _wasPlaying)//resume any looping sample that was playing before it was deactivated
				{
					Play();
				}
			}
		}
	}

	void OpenALSound::Deactivate()
	{
		if (_isActive) {
			if (_isLooping && IsPlaying()) {
				_wasPlaying = true;
			}
			else {
				_wasPlaying = false;
			}
			OpenALSoundSystem::InstancePtr()->ReleaseSource(_sourceId);
			_startPlaying = false;
			_isActive = false;
		}
	}

	const char *OpenALSound::GetName() const
	{ 
		return _name.c_str();
	}

	Vector *OpenALSound::GetPosition() const
	{
		return _position;
	}

	Vector *OpenALSound::GetVelocity() const{
		return _velocity;
	}

	float OpenALSound::GetInnerRange() const{
		return _innerRange;
	}

	void OpenALSound::SetInnerRange(float innerRange){ 
		_innerRange = innerRange;
	}

	float OpenALSound::GetOuterRange() const{ 
		return _outerRange;
	}

	void OpenALSound::SetOuterRange(float outerRange){
		_outerRange = outerRange;
	}

	bool OpenALSound::GetSourceRelative() const{
		return _isSourceRelative;
	}

	void OpenALSound::SetSourceRelative(bool sourceRelative)
	{ 
		_isSourceRelative = sourceRelative;
		if (_isActive) {
			if (_isSourceRelative) {
				alSourcei(_sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
				alSource3f(_sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
				alSource3f(_sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
			}
			else {
				alSourcei(_sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
				alSource3f(_sourceId,AL_POSITION,_position->X,_position->Y,_position->Z);
				alSource3f(_sourceId,AL_VELOCITY,_velocity->X,_velocity->Y,_velocity->Z);
			}
		}
	}

	float OpenALSound::GetVolume() const{
		return _volume;
	}

	float OpenALSound::GetAttenuatedVolume()
	{
		return _volume * _attenuationFactor;
	}

	void OpenALSound::SetVolume(float volume){
		_volume = volume;
		if (_isActive) {
			alSourcef(_sourceId,AL_GAIN,_volume * _globalVolume * _attenuationFactor);
		}
	}

	void OpenALSound::SetGlobalVolume(float globalVolume)
	{
		_globalVolume = globalVolume;
		if (_isActive) {
			SetVolume(_volume);
		}
	}

	void OpenALSound::Update(float attenuationFactor)
	{
		_attenuationFactor = attenuationFactor;

		if (_isActive) {
			SetVolume(_volume);
			alSource3f(_sourceId,AL_POSITION,_position->X,_position->Y,_position->Z);
			alSource3f(_sourceId,AL_VELOCITY,_velocity->X,_velocity->Y,_velocity->Z);
			if (_startPlaying) {
				_startPlaying = false;
				alSourcePlay(_sourceId);
			}
		}
	}

	float OpenALSound::GetPitch() const{ 
		return _pitch;
	}

	void OpenALSound::SetPitch(float pitch){
		_pitch = pitch;
		if (_isActive) {
			alSourcef(_sourceId,AL_PITCH,_pitch);
		}
	}

	void OpenALSound::SetPriority(int priority){
		_priority = priority;
	}

	int OpenALSound::GetPriority() const{
		return _priority;
	}

	bool OpenALSound::GetLooping() const{ 
		return _isLooping;
	}

	void OpenALSound::SetLooping(bool looping){
		_isLooping = looping;
		if (_isActive) {
			alSourcei (_sourceId, AL_LOOPING,  _isLooping ? AL_TRUE : AL_FALSE);
		}
	}

	void OpenALSound::Stop(){
		_wasPlaying = false;
		if (_isActive) {
			alSourceStop(_sourceId);
		}
	}

	void OpenALSound::Pause(){
		_wasPlaying = false;
		if (_isActive) {
			alSourcePause(_sourceId);
		}
	}

	void OpenALSound::Play(){
		if (_isActive) {
			_startPlaying = true;//start playing on next update so we can ensure the position/velocity/attenuation has been calculated before playing begins
		}
	}

	bool OpenALSound::IsStopped() const{
		ALint state;
		alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
		return state==AL_STOPPED;
	}

	bool OpenALSound::IsPaused() const{
		ALint state;
		alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
		return state==AL_PAUSED;
	}

	bool OpenALSound::IsPlaying() const{
		ALint state;
		alGetSourcei(_sourceId, AL_SOURCE_STATE, &state);
		return _startPlaying || state==AL_PLAYING;
	}

	bool OpenALSound::IsActive() const{
		return _isActive;
	}

}}}}