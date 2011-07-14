#pragma once

#include <MGDF/MGDFDisposable.hpp>

namespace MGDF {

/** 
this struct represents a vector in 3d space
*/
typedef struct
{
	float X;
	float Y;
	float Z;
} Vector;

/**
this class represents an audio source in 3d space
\author gcconner
*/
class ISound: public IDisposable {
public:
	virtual const wchar_t * GetName() const =0;
	virtual Vector * GetPosition() const =0;
	virtual Vector * GetVelocity() const =0;

	virtual float  GetInnerRange() const =0;
	virtual void  SetInnerRange(float sourceRelative)=0;
	virtual float  GetOuterRange() const =0;
	virtual void  SetOuterRange(float sourceRelative)=0;

	virtual bool  GetSourceRelative() const =0;
	virtual void  SetSourceRelative(bool sourceRelative)=0;

	virtual float  GetVolume() const =0;
	virtual void  SetVolume(float volume)=0;
	virtual float  GetPitch() const =0;
	virtual void  SetPitch(float pitch)=0;

	/**
	priority is used by the sound system to determine which sounds get deactivated if not enough sound sources
	are available to play all sounds
	*/
	virtual void  SetPriority(int priority)=0;
	virtual int  GetPriority() const =0;

	virtual bool  GetLooping() const =0;
	virtual void  SetLooping(bool looping)=0;

	/**
	stop the playing of the current playlist if it was playing or paused (resets playlist to the beginning aswell)
	*/
	virtual void  Stop()=0;

	/**
	pause the playing of the current playlist (if it was playing)
	*/
	virtual void  Pause()=0;

	/**
	resume the playing of the current playlist (if it was paused or stopped)
	*/
	virtual void  Play()=0;

	/**
	determines if the playlist has stopped play
	\return true if the playlist has stopped play
	*/
	virtual bool  IsStopped() const =0;

	/**
	determines if the playlist has paused play
	\return true if the playlist has paused play
	*/
	virtual bool  IsPaused() const =0;

	/**
	determines if the playlist is playing
	\return true if the playlist is playing
	*/
	virtual bool  IsPlaying() const =0;

	/**
	returns true if the sound manager has culled this sound source
	*/
	virtual bool  IsActive() const =0;
};

}