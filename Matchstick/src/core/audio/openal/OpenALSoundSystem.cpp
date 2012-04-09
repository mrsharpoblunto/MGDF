#include "StdAfx.h"

#include "AL/alut.h"
#include <boost/lexical_cast.hpp>

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFExceptions.hpp"

#include "OpenALSoundSystem.hpp"

#define MAX_SOURCES 256

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif


namespace MGDF { namespace core { namespace audio { namespace openal_audio {

DEFINE_SINGLETON(OpenALSoundSystem);

int OpenALSoundSystem::_refCount = 0;

OpenALSoundSystem *OpenALSoundSystem::SafeNew()
{
	if (_refCount++ == 0) {
		new OpenALSoundSystem();
	}
	return OpenALSoundSystem::InstancePtr();
}

void OpenALSoundSystem::SafeDelete()
{
	if (--_refCount ==0) {
		delete OpenALSoundSystem::InstancePtr();
	}
}

/**
initialise the openAL sound system
*/
OpenALSoundSystem::OpenALSoundSystem()
{
	ALCdevice *device = alcOpenDevice(NULL);
	if (device == NULL)
	{
		GetLoggerImpl()->Add(THIS_NAME,"Unable to open audio device",LOG_ERROR);
    	throw MGDFException("Unable to open audio device");
	}

	_context=alcCreateContext(device,NULL);
	if (_context == NULL)
	{
		GetLoggerImpl()->Add(THIS_NAME,"Failed to initialize OpenAL",LOG_ERROR);
    	throw MGDFException("Failed to initialize OpenAL");
	}

	alcMakeContextCurrent(_context);
	if (alcGetError(device) !=AL_NO_ERROR)
	{
		GetLoggerImpl()->Add(THIS_NAME,"Failed to make OpenAL context current",LOG_ERROR);
    	throw MGDFException("Failed to make OpenAL context current");
	}

	alutInitWithoutContext(NULL,NULL);

	//allocate as many sources as we can (or until we reach a hard coded limit) and add them to the free sources pool.
	do
	{
		ALuint sourceId;
		alGenSources(1,&sourceId);
		if (alGetError() != AL_NO_ERROR)
		{
			break;
		}
		else {
			_freeSources.push(sourceId);
		}
	}
	while (_freeSources.size()<MAX_SOURCES);

	GetLoggerImpl()->Add(THIS_NAME,"max audio sources determined - "+boost::lexical_cast<std::string>(_freeSources.size()),LOG_LOW);
	GetLoggerImpl()->Add(THIS_NAME,"initialised successfully",LOG_LOW);
}

int OpenALSoundSystem::GetFreeSources()
{
	return _freeSources.size();
}

bool OpenALSoundSystem::AcquireSource(ALuint *source)
{
	int freeSources = GetFreeSources();
	if (freeSources>0)
	{
		ALuint freeSource = _freeSources.top();
		_freeSources.pop();
		_allocatedSources[freeSource] = true;

		//make sure we clear out any properties from a source acquired from the pool
		alSourcei(freeSource, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(freeSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(freeSource, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSourcef(freeSource,AL_PITCH,1.0f);
		alSourcei(freeSource, AL_LOOPING, AL_FALSE);

		*source = freeSource;
		return true;
	}
	else {
		return false;
	}
}

void OpenALSoundSystem::ReleaseSource(ALuint source)
{
	boost::unordered_map<ALuint,bool>::iterator iter = _allocatedSources.find(source);
	if (iter!=_allocatedSources.end())
	{
		//stop the source, clear out its buffer and re add it to the free source pool.
		alSourceStop(source);
		alSourcei(source, AL_BUFFER,0);

		_allocatedSources.erase(iter);
		_freeSources.push(source);
	}
}

/**
uninitialise the openAL sound system
*/
OpenALSoundSystem::~OpenALSoundSystem()
{
	//Get device for active context
	ALCdevice *device=alcGetContextsDevice(_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(_context);
	alcCloseDevice(device);

	while (_freeSources.size()>0)
	{
		ALuint source = _freeSources.top();
		_freeSources.pop();
		alDeleteSources(1, &source);
	}

	GetLoggerImpl()->Add(THIS_NAME,"uninitialised successfully",LOG_LOW);
}

}}}}