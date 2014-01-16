#pragma once

#include <MGDF/MGDFError.hpp>

namespace MGDF
{

/**
this class represents an audio stream
*/
class ISoundStream
{
public:
	/**
	The name of the sound stream
	\return The name of the sound stream
	*/
	virtual const wchar_t * GetName() const = 0;

	/**
	Get the volume level (0-1)
	\return The volume level (0-1)
	*/
	virtual float  GetVolume() const = 0;
	
	/**
	Set the volume level (0-1)
	*/
	virtual void  SetVolume( float volume ) = 0;

	/**
	The position of playback in the stream (0 -> GetLength)
	\return The position of playback in the stream
	*/
	virtual UINT32 GetPosition() = 0;

	/**
	The total length of the stream
	\return The total length of the stream
	*/
	virtual UINT32 GetLength() = 0;

	/**
	removes the soundstream
	*/
	virtual void Dispose() = 0;

	/**
	stop the playing of the current stream if it was playing or paused (resets it to the beginning aswell)
	*/
	virtual void  Stop() = 0;

	/**
	pause the playing of the current stream (if it was playing)
	*/
	virtual void  Pause() = 0;

	/**
	resume the playing of the current playlist (if it was paused or stopped)
	\return MGDF_OK if the sound could resume playing, otherwise returns an error code.
	*/
	virtual MGDFError Play() = 0;

	/**
	determines if the stream has stopped play
	\return true if the stream has stopped play
	*/
	virtual bool  IsStopped() const = 0;

	/**
	determines if the stream has paused play
	\return true if the stream has paused play
	*/
	virtual bool  IsPaused() const = 0;

	/**
	determines if the stream is playing
	\return true if the stream is playing
	*/
	virtual bool  IsPlaying() const = 0;
};

}