#pragma once

#include <MGDF/MGDFString.hpp>

namespace MGDF {

/**
An audio stream
*/
MIDL_INTERFACE("2F0555E6-B5C3-40C4-9399-F74699A38811")
ISoundStream : public IUnknown {
 public:
  /**
  The name of the sound stream
  \param name pointer to the name of the sound stream
  \
  */
  virtual void STDMETHODCALLTYPE GetName(IWString * *name) = 0;

  /**
  Get the volume level (0-1)
  \return The volume level (0-1)
  */
  virtual float STDMETHODCALLTYPE GetVolume() const = 0;

  /**
  Set the volume level (0-1)
  \param volume the volume level to set
  */
  virtual void STDMETHODCALLTYPE SetVolume(float volume) = 0;

  /**
  The position of playback in the stream (0 -> GetLength)
  \return The position of playback in the stream
  */
  virtual UINT32 STDMETHODCALLTYPE GetPosition() = 0;

  /**
  The total length of the stream
  \return The total length of the stream
  */
  virtual UINT32 STDMETHODCALLTYPE GetLength() = 0;

  /**
  stop the playing of the current stream if it was playing or paused (resets it
  to the beginning aswell)
  */
  virtual void STDMETHODCALLTYPE Stop() = 0;

  /**
  pause the playing of the current stream (if it was playing)
  */
  virtual void STDMETHODCALLTYPE Pause() = 0;

  /**
  resume the playing of the current playlist (if it was paused or stopped)
  \return success if the sound could resume playing, otherwise returns an error
  code.
  */
  virtual HRESULT STDMETHODCALLTYPE Play() = 0;

  /**
  determines if the stream has stopped play
  \return true if the stream has stopped play
  */
  virtual bool STDMETHODCALLTYPE IsStopped() const = 0;

  /**
  determines if the stream has paused play
  \return true if the stream has paused play
  */
  virtual bool STDMETHODCALLTYPE IsPaused() const = 0;

  /**
  determines if the stream is playing
  \return true if the stream is playing
  */
  virtual bool STDMETHODCALLTYPE IsPlaying() const = 0;
};

}  // namespace MGDF