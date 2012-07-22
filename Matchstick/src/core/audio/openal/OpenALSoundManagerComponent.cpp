#include "StdAfx.h"

#include <math.h>
#include <algorithm>

#include <limits.h>
#include "al.h"
#include "alc.h"
#include "AL/alut.h"
#include "OpenALSound.hpp"
#include "VorbisStream.hpp"

#include "OpenALSoundManagerComponent.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace audio { namespace openal_audio {

ISoundManagerComponent *CreateOpenALSoundManagerComponent(HINSTANCE instance,IVirtualFileSystem *vfs,HWND window)
{
	try {
		return new OpenALSoundManagerComponentImpl(instance,vfs,window);
	}
	catch (...)
	{
		return nullptr;
	}
}

OpenALSoundManagerComponentImpl::OpenALSoundManagerComponentImpl(HINSTANCE instance,IVirtualFileSystem *vfs,HWND window):
	_vfs(vfs)
{
	_enableAttenuation = false;
	this->_context = OpenALSoundSystem::SafeNew()->GetContext();

	_orientationForward = XMFLOAT3(0.0f,0.0f,1.0f);
	_orientationUp		= XMFLOAT3(0.0f,1.0f,0.0f);
	_position			= XMFLOAT3(0.0f,0.0f,0.0f);
	_velocity			= XMFLOAT3(0.0f,0.0f,0.0f);

	alDistanceModel(AL_NONE);
}

void OpenALSoundManagerComponentImpl::Dispose()
{
	delete this;
}

OpenALSoundManagerComponentImpl::~OpenALSoundManagerComponentImpl()
{
	for (int i=_sounds.size()-1;i>=0;--i)
	{
		delete _sounds[i];
	}
	for (int i=_soundStreams.size()-1;i>=0;--i)
	{
		delete _soundStreams[i];
	}
	for (auto iter = _sharedBuffers.begin();iter!=_sharedBuffers.end();++iter)
	{
		alDeleteBuffers(1,&(iter->first));
		delete iter->second;
	}

	OpenALSoundSystem::SafeDelete();
}

void OpenALSoundManagerComponentImpl::Update()
{
	alListener3f(AL_POSITION,_position.x,_position.y,_position.z);
	alListener3f(AL_VELOCITY,_velocity.x,_velocity.y,_velocity.z);

	float orientation[6];
	orientation[0] = _orientationForward.x; //forward vector x value
	orientation[1] = _orientationForward.y; //forward vector y value
	orientation[2] = _orientationForward.z; //forward vector z value
	orientation[3] = _orientationUp.x; //up vector x value
	orientation[4] = _orientationUp.y; //up vector y value
	orientation[5] = _orientationUp.z; //up vector z value
	alListenerfv(AL_ORIENTATION, orientation);

	int deactivatedSoundsCount=0;

	for (auto iter = _sounds.begin();iter!=_sounds.end();++iter)
	{
		OpenALSound *sound = (OpenALSound *)*iter;
		if (!sound->IsActive()) {
			deactivatedSoundsCount++;
		}

		float attenuation=1;
		if (_enableAttenuation)
		{
			//work out the sounds attenuation due to distance
			
			XMVECTOR distanceVector = XMVectorSet(_position.x - sound->GetPosition()->x,_position.y - sound->GetPosition()->y,_position.z - sound->GetPosition()->z,1.0f);
			float distance = XMVectorGetX(XMVector3Length(distanceVector));

			if (distance<=sound->GetInnerRange()) 
			{
				attenuation = 1;
			}
			else if (distance>=sound->GetOuterRange())
			{
				attenuation = 0;
			}
			else {
				attenuation = 1-((distance-sound->GetInnerRange())/(sound->GetOuterRange()-sound->GetInnerRange()));
			}
		}
		sound->Update(attenuation);
	}

	if (deactivatedSoundsCount>0)
	{
		PrioritizeSounds(deactivatedSoundsCount);
	}

	for (std::vector<ISoundStream *>::iterator iter = _soundStreams.begin();iter!=_soundStreams.end();++iter)
	{
		VorbisStream *stream = (VorbisStream *)*iter;
		stream->Update();
	}
}

XMFLOAT3 *OpenALSoundManagerComponentImpl::GetListenerOrientationForward()
{
	return &_orientationForward;
}

XMFLOAT3 *OpenALSoundManagerComponentImpl::GetListenerOrientationUp()
{
	return &_orientationUp;
}

XMFLOAT3 *OpenALSoundManagerComponentImpl::GetListenerPosition()
{
	return &_position;
}

XMFLOAT3 *OpenALSoundManagerComponentImpl::GetListenerVelocity()
{
	return &_velocity;
}

bool OpenALSoundManagerComponentImpl::GetEnableAttenuation() const
{
	return _enableAttenuation;
}

void OpenALSoundManagerComponentImpl::SetEnableAttenuation(bool enableAttenuation)
{
	_enableAttenuation = enableAttenuation;
}

float OpenALSoundManagerComponentImpl::GetSoundVolume() const
{
	return _soundVolume;
}

void OpenALSoundManagerComponentImpl::SetSoundVolume(float volume)
{
	_soundVolume = volume;
	for (auto iter = _sounds.begin();iter!=_sounds.end();++iter)
	{
		OpenALSound *sound = (OpenALSound *)*iter;
		sound->SetGlobalVolume(_soundVolume);
	}
}

float OpenALSoundManagerComponentImpl::GetStreamVolume() const
{
	return _streamVolume;
}

void OpenALSoundManagerComponentImpl::SetStreamVolume(float volume)
{
	_streamVolume = volume;
	for (auto iter = _soundStreams.begin();iter!=_soundStreams.end();++iter)
	{
		VorbisStream *stream = (VorbisStream *)*iter;
		stream->SetGlobalVolume(_streamVolume);
	}
}

float OpenALSoundManagerComponentImpl::GetDopplerShiftFactor() const
{
	return alGetFloat(AL_DOPPLER_FACTOR);
}

void OpenALSoundManagerComponentImpl::SetDopplerShiftFactor(float dopplerShiftFactor)
{
	alDopplerFactor(dopplerShiftFactor);
}

float OpenALSoundManagerComponentImpl::GetSpeedOfSound() const
{
	return alGetFloat(AL_SPEED_OF_SOUND);
}

void OpenALSoundManagerComponentImpl::SetSpeedOfSound(float speedOfSound)
{
	alSpeedOfSound(speedOfSound);
}

ISoundStream *OpenALSoundManagerComponentImpl::CreateSoundStream(IFile *file)
{
	//try to deactivate a sound in order to free up a source for the new sound
	//however it may not be possible to deactivate any of the current sounds so the
	//new sound may have to be created as inactive.
	if (OpenALSoundSystem::InstancePtr()->GetFreeSources()==0)
	{
		DeactivateSound(INT_MAX);
	}

	//if we couldn't deactivate any sources then we cannot create the stream
	if (OpenALSoundSystem::InstancePtr()->GetFreeSources()==0)
	{
		GetComponentErrorHandler()->SetLastError(THIS_NAME,MGDF_ERR_NO_FREE_SOURCES,"No free sound sources to create stream");
		return nullptr;
	}
	else 
	{
		try {
			ISoundStream *stream = new VorbisStream(file,this);
			_soundStreams.push_back(stream);
			return stream;
		}
		catch (...)
		{
			return nullptr;
		}
	}
}

ISound *OpenALSoundManagerComponentImpl::CreateSound(IFile *file, int priority)
{
	//try to deactivate a sound in order to free up a source for the new sound
	//however it may not be possible to deactivate any of the current sounds so the
	//new sound may have to be created as inactive.
	if (OpenALSoundSystem::InstancePtr()->GetFreeSources()==0)
	{
		DeactivateSound(priority);
	}

	try {
		ISound *sound = new OpenALSound(file,this,priority);
		_sounds.push_back(sound);
		return sound;
	}
	catch (...)
	{
		return nullptr;
	}

}

void OpenALSoundManagerComponentImpl::DeactivateSound(int priority)
{
	//find all sounds with a priority equal or lower to the one to be created
	std::vector<OpenALSound *> sounds;
	for (std::vector<ISound *>::iterator iter = _sounds.begin();iter!=_sounds.end();++iter) {
		if ((*iter)->GetPriority()<=priority)
		{
			sounds.push_back((OpenALSound *)(*iter));
		}
	}
	if (sounds.size()>0) {
		sort(sounds.begin(),sounds.end(),&OpenALSoundManagerComponentImpl::Sort);
		sounds[0]->Deactivate();
	}
}

//ensure as many samples are active as possible, with the highest precedence samples being activated first
void OpenALSoundManagerComponentImpl::PrioritizeSounds(int deactivatedSoundsCount)
{
	//copy the sounds into a local list so sorting won't mess up the external ordering of the samples
	std::vector<OpenALSound *> sounds;
	for (auto iter = _sounds.begin();iter!=_sounds.end();++iter)
	{
		sounds.push_back((OpenALSound *)(*iter));
	}
	sort(sounds.begin(),sounds.end(),&OpenALSoundManagerComponentImpl::Sort);

	//detect how many samples will need to be deactivated
	int freeSources = OpenALSoundSystem::InstancePtr()->GetFreeSources();
	int requiringDeactivationCount = deactivatedSoundsCount - freeSources;
	if (requiringDeactivationCount<0) requiringDeactivationCount = 0;//we can activate all sounds.

	//deactivate requiringDeactivationCount of the lowest priority samples, and reactivate the rest.
	for (std::vector<OpenALSound *>::iterator iter = sounds.begin();iter!=sounds.end();++iter)
	{
		if (requiringDeactivationCount-->0) {
			if ((*iter)->IsActive()) {
				(*iter)->Deactivate();
			}
		}
		else {
			if (!(*iter)->IsActive()) {
				(*iter)->Reactivate();
			}
		}
	}
}

//sort sounds into the lowest->highest priority
bool OpenALSoundManagerComponentImpl::Sort(OpenALSound *a,OpenALSound *b)
{
	if (a->GetPriority()<b->GetPriority()) {
		return true;
	}
	else if (a->GetAttenuatedVolume()<b->GetAttenuatedVolume()) {
		return true;
	}
	else if (!a->GetLooping() && b->GetLooping())
	{
		return true;
	}
	else {
		return false;
	}
}

ALuint OpenALSoundManagerComponentImpl::GetSoundBuffer(IFile *dataSource)
{
	std::wstring dataSourceName = dataSource->GetLogicalPath();
	dataSourceName.append(VFS_PATH_SEPARATOR);
	dataSourceName.append(dataSource->GetName());

	//see if the buffer already exists in memory before trying to create it
	for (auto iter = _sharedBuffers.begin();iter!=_sharedBuffers.end();++iter)
	{
		if (iter->second->BufferSource==dataSourceName) {
			++iter->second->References;
			return iter->first;
		}
	}

	//if the buffer doesn't exist, then load it off the vfs.
	if (dataSource->IsOpen()) {
		dataSource->SetPosition(0);
	}
	else {
		dataSource->OpenFile();
	}
	int size = dataSource->GetSize();
	char *data = new char[size];
	dataSource->Read((void *)data,size);
	dataSource->CloseFile();

	ALuint bufferId = alutCreateBufferFromFileImage((ALvoid *)data,size);
	delete[] data;

	//if the buffer loaded ok, add it to the list of loaded shared buffers
	if (bufferId != ALUT_ERROR_AL_ERROR_ON_ENTRY && bufferId != ALUT_ERROR_ALC_ERROR_ON_ENTRY) {
		SharedBuffer *sharedBuffer = new SharedBuffer();
		sharedBuffer->BufferSource = dataSourceName;
		sharedBuffer->References = 1;
		_sharedBuffers[bufferId] = sharedBuffer;
	}
	return bufferId;
}

void OpenALSoundManagerComponentImpl::RemoveSoundBuffer(ALuint bufferId)
{
	if (_sharedBuffers.find(bufferId)!=_sharedBuffers.end()) {
		--_sharedBuffers[bufferId]->References;
		//if there are no more references to this buffer, remove it.
		if (_sharedBuffers[bufferId]->References==0)
		{
			alDeleteBuffers(1,&bufferId);
			delete _sharedBuffers[bufferId];
			_sharedBuffers.erase(bufferId);
		}
	}
}

void OpenALSoundManagerComponentImpl::RemoveSound(ISound *sound)
{
	delete sound;
}

void OpenALSoundManagerComponentImpl::RemoveSoundStream(ISoundStream *stream)
{
	delete stream;
}

void OpenALSoundManagerComponentImpl::DoRemoveSoundStream(ISoundStream *stream)
{
	auto iter = find(_soundStreams.begin(), _soundStreams.end(), stream);
	if (iter!=_soundStreams.end())
	{
		_soundStreams.erase(iter);
	}
}

void OpenALSoundManagerComponentImpl::DoRemoveSound(ISound *sound)
{
	auto iter = find(_sounds.begin(), _sounds.end(), sound);
	if (iter!=_sounds.end())
	{
		_sounds.erase(iter);
	}
}

}}}}