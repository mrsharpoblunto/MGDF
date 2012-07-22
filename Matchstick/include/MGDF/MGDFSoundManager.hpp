#pragma once

#include <xnamath.h>
#include <MGDF/MGDFSound.hpp>
#include <MGDF/MGDFSoundStream.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
 this interface provides the functionality for processing sounds in 3d space
 \author gcconner
*/
class ISoundManager {
public:
	/**
	determine the position/velocity of the observe of the sound
	*/
	virtual XMFLOAT3 * GetListenerPosition()=0;
	virtual XMFLOAT3 * GetListenerVelocity()=0;
	virtual XMFLOAT3 * GetListenerOrientationForward()=0;
	virtual XMFLOAT3 * GetListenerOrientationUp()=0;

	virtual float  GetSoundVolume() const=0;
	virtual void  SetSoundVolume(float volume)=0;

	virtual float  GetStreamVolume() const=0;
	virtual void  SetStreamVolume(float volume)=0;

	virtual bool  GetEnableAttenuation() const =0;   
	virtual void  SetEnableAttenuation(bool enableAttenuation)=0;

	virtual float  GetDopplerShiftFactor() const =0;
	virtual void  SetDopplerShiftFactor(float dopplerShiftFactor)=0;

	virtual float  GetSpeedOfSound() const =0;
	virtual void  SetSpeedOfSound(float speedOfSound)=0;

	/**
	create a sound from a file in the VFS returns nullptr if the sound could not be created (see system->GetLastError() to get details as to why it failed)
	\param file the data source for the sound
	*/
	virtual ISound * CreateSound(IFile *file, int priority)=0;

	/**
	create a sound stream from a file in the VFS. If no sound sources are available, the sound manager
	will deactivate a sound source if possible, however if no sources are available then this function returns nullptr 
	also returns nullptr if the stream could not be created (see system->GetLastError() to get details as to why it failed)
	\param file the data source for the sound
	*/
	virtual ISoundStream * CreateSoundStream(IFile *file)=0;

	virtual void RemoveSound(ISound *sound)=0;
	virtual void RemoveSoundStream(ISoundStream *soundStream)=0;
};

}