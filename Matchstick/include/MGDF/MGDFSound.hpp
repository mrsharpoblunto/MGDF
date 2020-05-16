#pragma once

#include <DirectXMath.h>
#include <windows.h>

namespace MGDF {

/**
An audio source in 3d space
*/
MIDL_INTERFACE("EBCC1B73-815F-4870-BC80-67A5379E65FC")
ISound : public IUnknown {
 public:
  /**
  The name of the sound
  \return the name of the sound
  */
  virtual const wchar_t* GetName() const = 0;

  /**
  The position of the sound
  \return the position of the sound
  */
  virtual DirectX::XMFLOAT3* GetPosition() = 0;

  /**
  The velocity of the sound
  \return the velocity of the sound
  */
  virtual DirectX::XMFLOAT3* GetVelocity() = 0;

  /**
  The distance (from the observer) at which the sound intensity reaches its
  maximum. Getting closer than this will not cause the sound intensity to get
  any greater \return The distance (from the observer) at which the sound
  intensity reaches its maximum. Getting closer than this will not cause the
  sound intensity to get any greater
   */
  virtual float GetInnerRange() const = 0;

  /**
   Set The distance (from the observer) at which the sound intensity reaches its
   maximum. Getting closer than this will not cause the sound intensity to get
   any greater \param innerRange The distance (from the observer) at which the
   sound intensity reaches its maximum. Getting closer than this will not cause
   the sound intensity to get any greater
   */
  virtual void SetInnerRange(float innerRange) = 0;

  /**
  The distance (from the observer) at which the sound intensity reaches 0
   \return The distance (from the observer) at which the sound intensity reaches
  its 0
   */
  virtual float GetOuterRange() const = 0;

  /**
   Set The distance (from the observer) at which the sound intensity reaches 0
   \param outerRange The distance (from the observer) at which the sound
   intensity reaches 0
   */
  virtual void SetOuterRange(float outerRange) = 0;

  /**
   Does the sound stay relative to the listeners position or is it fixed in
   absolute space \return true if the sound is always relative to the listeners
   position
   */
  virtual bool GetSourceRelative() const = 0;

  /**
   Does the sound stay relative to the listeners position or is it fixed in
   absolute space \param sourceRelative true if the sound is always relative to
   the listeners position
   */
  virtual void SetSourceRelative(bool sourceRelative) = 0;

  /**
  Get the current volume of the sound (ignoring attenuation due to position)
  \return the current volume of the sound (ignoring attenuation due to position)
  */
  virtual float GetVolume() const = 0;

  /**
  Set the current volume of the sound (ignoring attenuation due to position)
  \param volume the current volume of the sound (ignoring attenuation due to
  position)
  */
  virtual void SetVolume(float volume) = 0;

  /**
  Get the pitch of the current sound
  \return 1.0 if the pitch is unchanged. between 0.5 and 1.0 is lower, and
  between 1.0 and 2.0 is higher pitched
  */
  virtual float GetPitch() const = 0;

  /**
  Set the pitch of the current sound
  \param pitch 1.0 if the pitch is unchanged. between 0.5 and 1.0 is lower, and
  between 1.0 and 2.0 is higher pitched
  */
  virtual void SetPitch(float pitch) = 0;

  /**
  Set the priority used by the sound system to determine which sounds get
  deactivated if not enough sound sources are available to play all sounds
  \param priority the priority to use for the sound. Higher values mean higher
  priority
  */
  virtual void SetPriority(INT32 priority) = 0;

  /**
  Get the priority used by the sound system to determine which sounds get
  deactivated if not enough sound sources are available to play all sounds
  \return priority the priority to use for the sound. Higher values mean higher
  priority
  */
  virtual INT32 GetPriority() const = 0;

  /**
   Get whether the sound will loop playback continously
   \return true if the sound should loop continously
   */
  virtual bool GetLooping() const = 0;

  /**
   Set whether the sound will loop playback continously
   \param looping true if the sound should loop playback, false otherwise
   \return looping true if the sound should loop continously
   */
  virtual void SetLooping(bool looping) = 0;

  /**
  stop the playing of the current playlist if it was playing or paused (resets
  playlist to the beginning aswell)
  */
  virtual void Stop() = 0;

  /**
  pause the playing of the current playlist (if it was playing)
  */
  virtual void Pause() = 0;

  /**
  resume the playing of the current playlist (if it was paused or stopped)
  */
  virtual void Play() = 0;

  /**
  determines if the playlist has stopped play
  \return true if the playlist has stopped play
  */
  virtual bool IsStopped() const = 0;

  /**
  determines if the playlist has paused play
  \return true if the playlist has paused play
  */
  virtual bool IsPaused() const = 0;

  /**
  determines if the playlist is playing
  \return true if the playlist is playing
  */
  virtual bool IsPlaying() const = 0;

  /**
  returns true if the sound manager has culled this sound source
  */
  virtual bool IsActive() const = 0;
};

}  // namespace MGDF