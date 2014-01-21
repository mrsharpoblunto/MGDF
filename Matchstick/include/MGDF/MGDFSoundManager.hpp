#pragma once

#include <DirectXMath.h>
#include <MGDF/MGDFSound.hpp>
#include <MGDF/MGDFSoundStream.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFList.hpp>

namespace MGDF
{

/**
 this interface provides the functionality for processing sounds in 3d space
*/
class ISoundManager
{
public:
	/**
	determine the position of the listener (i.e. the player)
	\return the position of the listener (i.e. the player)
	*/
	virtual DirectX::XMFLOAT3 * GetListenerPosition() = 0;

	/**
	determine the velocity of the listener (i.e. the player)
	\return the velocity of the listener (i.e. the player)
	*/
	virtual DirectX::XMFLOAT3 * GetListenerVelocity() = 0;

	/**
	 Get the vector representing the forward direction of the listener (i.e. the player)
	 \return the vector representing the forward direction of the listener (i.e. the player)
	 */
	virtual DirectX::XMFLOAT3 * GetListenerOrientationForward() = 0;

	/**
	 Get the vector representing the upward direction of the listener (i.e. the player)
	 \return the vector representing the upward direction of the listener (i.e. the player)
	 */
	virtual DirectX::XMFLOAT3 * GetListenerOrientationUp() = 0;

	/**
	Get the master volume (0-1) All sounds volumes are relative to this setting
	\return the master volume
	*/
	virtual float  GetSoundVolume() const = 0;

	/**
	Set the master volume (0-1) All sounds volumes are relative to this setting
	\param volume the master volume
	*/
	virtual void  SetSoundVolume( float volume ) = 0;

	/**
	Get the master volume for streamed sounds (0-1) All streams volumes are relative to this setting
	\return the master stream volume
	*/
	virtual float  GetStreamVolume() const = 0;

	/**
	Set the master volume for streamed sounds (0-1) All streams volumes are relative to this setting
	\param volume the master stream volume
	*/
	virtual void  SetStreamVolume( float volume ) = 0;

	/**
	Get whether sounds should attenuate with thier distance from the listener
	\return true if sounds should attenuate with thier distance from the listener
	*/
	virtual bool  GetEnableAttenuation() const = 0;

	/**
	Set whether sounds should attenuate with thier distance from the listener
	\param enableAttenuation true if sounds should attenuate with thier distance from the listener
	*/
	virtual void  SetEnableAttenuation( bool enableAttenuation ) = 0;

	/**
	Get whether sounds will doppler shift based on thier relative velocity to the listener
	\return whether sounds will doppler shift based on thier relative velocity to the listener
	*/
	virtual float  GetDopplerShiftFactor() const = 0;
	
	/**
	Set whether sounds will doppler shift based on thier relative velocity to the listener
	\param dopplerShiftFactor true if sounds should doppler shift based on thier relative velocity to the listener
	*/
	virtual void  SetDopplerShiftFactor( float dopplerShiftFactor ) = 0;

	/**
	Get the speed of sound used to calculate doppler shifting of sounds
	\return the speed of sound used to calculate doppler shifting of sounds
	*/
	virtual float  GetSpeedOfSound() const = 0;

	/**
	Set the speed of sound used to calculate doppler shifting of sounds
	\param speedOfSound the speed of sound used to calculate doppler shifting of sounds
	*/
	virtual void  SetSpeedOfSound( float speedOfSound ) = 0;

	/**
	create a sound by loading a file in the VFS into memory. When no longer used it should be Released
	\param file the data source for the sound
	\param priority the priority of the sound (used to determine what should play if no free audio sources are available
	\param sound If the sound is created successfully, this will point to the created sound
	\return MGDF_OK if the sound was created successfully, otherwise an error code will be returned
	*/
	virtual MGDFError CreateSound( IFile *file, INT32 priority, ISound **sound ) = 0;

	/**
	create a sound stream from a file in the VFS. When no longer used it should be Released
	\param file the data source for the sound stream
	\param stream If the stream is created successfully, this will point to the created stream
	\return MGDF_OK if the stream was created successfully, otherwise an error code will be returned
	*/
	virtual MGDFError CreateSoundStream( IFile *file, ISoundStream **stream ) = 0;
};

}