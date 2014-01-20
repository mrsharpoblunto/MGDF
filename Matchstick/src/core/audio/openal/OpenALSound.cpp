#include "StdAfx.h"

#include "../../common/MGDFLoggerImpl.hpp"
#include "OpenALSoundSystem.hpp"
#include "OpenALSound.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#pragma warning(disable:4345) //disable irrelevant warning about initializing POD types via new() syntax.

using namespace DirectX;

namespace MGDF
{
namespace core
{
namespace audio
{
namespace openal_audio
{

MGDFError OpenALSound::TryCreate( IFile *source, OpenALSoundManagerComponentImpl *manager, INT32 priority, OpenALSound **sound )
{
	*sound = new OpenALSound( manager, priority );
	MGDFError error = (*sound)->Init( source );
	if ( MGDF_OK != error ) {
		delete *sound;
		*sound = nullptr;
	}
	return error;
}

OpenALSound::OpenALSound( OpenALSoundManagerComponentImpl *manager, INT32 priority )
	: _soundManager( manager )
	, _priority( priority )
	, _position( XMFLOAT3( 0.0f, 0.0f, 0.0f ) )
	, _velocity( XMFLOAT3( 0.0f, 0.0f, 0.0f ) )
	, _innerRange( 0 )
	, _outerRange( 1 )
	, _volume( 1 )
	, _globalVolume( manager->GetSoundVolume() )
	, _attenuationFactor( 1 )
	, _pitch( 1 )
	, _isLooping( false )
	, _isSourceRelative( true )
	, _wasPlaying( false )
	, _startPlaying( false )
{
	_ASSERTE( manager );
}

MGDFError OpenALSound::Init( IFile *source )
{
	_ASSERTE( source );
	_name = source->GetName();

	MGDFError error = _soundManager->CreateSoundBuffer( source, &_bufferId );
	if ( MGDF_OK != error ) {
		return error;
	}
	Reactivate();
	return MGDF_OK;
}

OpenALSound::~OpenALSound()
{
	_soundManager->RemoveSound( this );
	_soundManager->RemoveSoundBuffer( _bufferId );
	Deactivate();
}

void OpenALSound::Dispose()
{
	delete this;
}

void OpenALSound::Reactivate()
{
	if ( MGDF_OK == _soundManager->AcquireSource( &_sourceId ) ) {
		alSourcei( _sourceId, AL_BUFFER, _bufferId );
		if ( alGetError() != AL_NO_ERROR ) {
			LOG( "Unable to allocate buffer to audio source", LOG_ERROR );
			Deactivate();
		} else {
			SetSourceRelative( _isSourceRelative );
			SetVolume( _volume );
			SetPitch( _pitch );
			SetLooping( _isLooping );
			if ( _isLooping && _wasPlaying ) { //resume any looping sample that was playing before it was deactivated
				Play();
			}
		}
	}
}

void OpenALSound::Deactivate()
{
	if ( _isActive ) {
		if ( _isLooping && IsPlaying() ) {
			_wasPlaying = true;
		} else {
			_wasPlaying = false;
		}
		_soundManager->ReleaseSource( _sourceId );
		_startPlaying = false;
		_isActive = false;
	}
}

void OpenALSound::SetSourceRelative( bool sourceRelative )
{
	_isSourceRelative = sourceRelative;
	if ( _isActive ) {
		if ( _isSourceRelative ) {
			alSourcei( _sourceId, AL_SOURCE_RELATIVE, AL_TRUE );
			alSource3f( _sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f );
			alSource3f( _sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f );
		} else {
			alSourcei( _sourceId, AL_SOURCE_RELATIVE, AL_FALSE );
			alSource3f( _sourceId, AL_POSITION, _position.x, _position.y, _position.z );
			alSource3f( _sourceId, AL_VELOCITY, _velocity.x, _velocity.y, _velocity.z );
		}
	}
}

const wchar_t *OpenALSound::GetName() const
{
	return _name;
}

XMFLOAT3 *OpenALSound::GetPosition()
{
	return &_position;
}

XMFLOAT3 *OpenALSound::GetVelocity()
{
	return &_velocity;
}

float OpenALSound::GetInnerRange() const
{
	return _innerRange;
}

void OpenALSound::SetInnerRange( float innerRange )
{
	_innerRange = innerRange;
}

float OpenALSound::GetOuterRange() const
{
	return _outerRange;
}

void OpenALSound::SetOuterRange( float outerRange )
{
	_outerRange = outerRange;
}

bool OpenALSound::GetSourceRelative() const
{
	return _isSourceRelative;
}

float OpenALSound::GetVolume() const
{
	return _volume;
}

float OpenALSound::GetAttenuatedVolume() const
{
	return _volume * _attenuationFactor;
}

void OpenALSound::SetVolume( float volume )
{
	_volume = volume;
	if ( _isActive ) {
		alSourcef( _sourceId, AL_GAIN, _volume * _globalVolume * _attenuationFactor );
	}
}

void OpenALSound::SetGlobalVolume( float globalVolume )
{
	_globalVolume = globalVolume;
	if ( _isActive ) {
		SetVolume( _volume );
	}
}

void OpenALSound::Update( float attenuationFactor )
{
	_attenuationFactor = attenuationFactor;

	if ( _isActive ) {
		SetVolume( _volume );
		alSource3f( _sourceId, AL_POSITION, _position.x, _position.y, _position.z );
		alSource3f( _sourceId, AL_VELOCITY, _velocity.x, _velocity.y, _velocity.z );
		if ( _startPlaying ) {
			_startPlaying = false;
			alSourcePlay( _sourceId );
		}
	}
}

float OpenALSound::GetPitch() const
{
	return _pitch;
}

void OpenALSound::SetPitch( float pitch )
{
	_pitch = pitch;
	if ( _isActive ) {
		alSourcef( _sourceId, AL_PITCH, _pitch );
	}
}

void OpenALSound::SetPriority( INT32 priority )
{
	_priority = priority;
}

INT32 OpenALSound::GetPriority() const
{
	return _priority;
}

bool OpenALSound::GetLooping() const
{
	return _isLooping;
}

void OpenALSound::SetLooping( bool looping )
{
	_isLooping = looping;
	if ( _isActive ) {
		alSourcei( _sourceId, AL_LOOPING,  _isLooping ? AL_TRUE : AL_FALSE );
	}
}

void OpenALSound::Stop()
{
	_wasPlaying = false;
	if ( _isActive ) {
		alSourceStop( _sourceId );
	}
}

void OpenALSound::Pause()
{
	_wasPlaying = false;
	if ( _isActive ) {
		alSourcePause( _sourceId );
	}
}

void OpenALSound::Play()
{
	if ( _isActive ) {
		_startPlaying = true;//start playing on next update so we can ensure the position/velocity/attenuation has been calculated before playing begins
	}
}

bool OpenALSound::IsStopped() const
{
	ALint state;
	alGetSourcei( _sourceId, AL_SOURCE_STATE, &state );
	return state == AL_STOPPED;
}

bool OpenALSound::IsPaused() const
{
	ALint state;
	alGetSourcei( _sourceId, AL_SOURCE_STATE, &state );
	return state == AL_PAUSED;
}

bool OpenALSound::IsPlaying() const
{
	ALint state;
	alGetSourcei( _sourceId, AL_SOURCE_STATE, &state );
	return _startPlaying || state == AL_PLAYING;
}

bool OpenALSound::IsActive() const
{
	return _isActive;
}

}
}
}
}