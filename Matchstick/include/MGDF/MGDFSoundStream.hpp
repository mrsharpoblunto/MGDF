#pragma once 

#include <MGDF/MGDFDisposable.hpp>

namespace MGDF {

/**
this class represents an audio stream
\author gcconner
*/
	class ISoundStream: public IDisposable {
public:
	virtual const wchar_t * GetName() const =0;

	virtual float  GetVolume() const =0;
	virtual void  SetVolume(float volume)=0;

	virtual unsigned int GetPosition() = 0;
	virtual unsigned int GetLength() =0;

	/**
	stop the playing of the current stream if it was playing or paused (resets it to the beginning aswell)
	*/
	virtual void  Stop()=0;

	/**
	pause the playing of the current stream (if it was playing)
	*/
	virtual void  Pause()=0;

	/**
	resume the playing of the current playlist (if it was paused or stopped)
	*/
	virtual void  Play()=0;

	/**
	determines if the stream has stopped play
	\return true if the stream has stopped play
	*/
	virtual bool  IsStopped() const =0;

	/**
	determines if the stream has paused play
	\return true if the stream has paused play
	*/
	virtual bool  IsPaused() const =0;

	/**
	determines if the stream is playing
	\return true if the stream is playing
	*/
	virtual bool  IsPlaying() const =0;
};

}