#pragma once

#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
The display settings for a particular adaptor mode
*/
struct AdaptorMode {
  UINT32 Width;
  UINT32 Height;
  UINT32 RefreshRateNumerator;
  UINT32 RefreshRateDenominator;
};

/**
The current fullscreen options
*/
struct FullScreenDesc {
  bool FullScreen;
  bool ExclusiveMode;
};

/**
Provides an interface for getting and setting the hosts various display settings
*/
MIDL_INTERFACE("BAF10DDF-1874-4299-A497-1F62C0691C8B")
IRenderSettingsManager : public IUnknown {
 public:
  /**
  gets the current vsync setting
  \return true if vsync is enabled
  */
  virtual bool GetVSync() const = 0;

  /**
  sets the current vsync setting
  \param vsync true if vsync is to be enabled
  */
  virtual void SetVSync(bool vsync) = 0;

  /**
  gets the current fullscreen setting
  \param fullscreen a description of the current fullscreen settings
  */
  virtual void GetFullscreen(FullScreenDesc * fullscreen) const = 0;

  /**
  sets the current fullscreen setting
  \param fullscreen a description of the desired fullscreen settings
  */
  virtual void SetFullscreen(const FullScreenDesc *fullscreen) = 0;

  /**
  get the number of supported multisample levels
  \return the number of supported multisample levels
  */
  virtual UINT32 GetMultiSampleLevelCount() const = 0;

  /**
  get the available multisample level supported by the display adaptor at the
  given index ( 0 to GetMultiSampleLevelCount() - 1 )
  \param index the
  multisample index
  \param level returns the multisample level at the given
  index
  \return true if a supported multisample level is present at the given
  index, false otherwise.
  */
  virtual bool GetMultiSampleLevel(UINT32 index, UINT32 * level) const = 0;

  /**
  set the display adaptors current multisample level, this changed setting is
  not applied until ApplySettings is called. \param multisampleLevel the desired
  multisample level for the backbuffer \return returns false if the desired
  multisample level cannot be set.
  */
  virtual bool SetBackBufferMultiSampleLevel(UINT32 multisampleLevel) = 0;

  /**
  get the current multisample level in use by the adaptor
  \return the current multisample level in use by the adaptor
  */
  virtual UINT32 GetBackBufferMultiSampleLevel() const = 0;

  /**
  set the desired multisample level for off screen render targets. This setting
  is not used directly by the framework but any client code should query this
  property when creating render targets that may require multisampling (see also
  GetCurrentMultiSampleLevel() ) \param multisampleLevel the desired multisample
  level for off screen render targets \return returns false if the desired
  multisample level cannot be set.
  */
  virtual bool SetCurrentMultiSampleLevel(UINT32 multisampleLevel) = 0;

  /**
  get the current desired multisample level for off screen render targets
  \param quality if specified this parameter will be initialized with the
  maximum multisampling quality setting for the current multisample level
  \return the current desired multisample level for off screen render targets
  */
  virtual UINT32 GetCurrentMultiSampleLevel(UINT32 * quality) const = 0;

  /**
  get the number of supported adaptor modes
  \return the number of supported adaptor modes
  */
  virtual UINT32 GetAdaptorModeCount() const = 0;

  /**
  get a supported adaptor mode at the given index (0 to GetAdaptorModeCount - 1
  ) \param index the adaptor mode index \param mode will be set to the the
  adaptor mode at the given index \return true if an adaptor mode exists at a
  particular index
  */
  virtual bool GetAdaptorMode(UINT32 index, AdaptorMode * mode) const = 0;

  /**
  get the adaptor mode (if any) matching the requested width and height, if no
  matching adaptor is found, false is returned \param width the desired width of
  the adaptor mode \param height the desired height of the adaptor mode \param
  mode will be set to the matching adaptor mode found (if any) \return true if a
  supported adaptor mode exists for the given width and height
  */
  virtual bool GetAdaptorMode(UINT32 width, UINT32 height, AdaptorMode * mode)
      const = 0;

  /**
  get the current adaptor mode being used when running in fullscreen mode
  \param mode will be set to the current adaptor mode being used.
  */
  virtual void GetCurrentAdaptorMode(AdaptorMode * mode) const = 0;

  /**
  sets the current display adaptor mode when running in fullscreen mode, this
  changed setting is not applied until ApplyChanges is called. \param mode the
  adaptor mode to use \return true if the adaptor mode is supported and can be
  applied, false otherwise
  */
  virtual bool SetCurrentAdaptorMode(const AdaptorMode *mode) = 0;

  /**
  sets the current display adaptor mode when running in fullscreen mode to the
  screens native fullscreen resolution. GetCurrentAdaptorMode should be called
  afterward to determine what adaptor was chosen \return true if the adaptor
  mode could be changed, false otherwise
  */
  virtual bool SetCurrentAdaptorModeToNative() = 0;

  /**
  sets the size of the window when running in windowed mode. While running in
  fullscreen, this function has no effect. \param width the width of the window
  \param height the height of the window
  */
  virtual void SetWindowSize(UINT32 width, UINT32 height) const = 0;

  /**
  get the current screen width, based on the current adaptor mode in fullscreen,
  or on the window dimensions otherwise \return the current screen width
  */
  virtual UINT32 GetScreenX() const = 0;

  /**
  get the current screen height, based on the current adaptor mode in
  fullscreen, or on the window dimensions otherwise \return thr current screen
  height
  */
  virtual UINT32 GetScreenY() const = 0;

  /**
  Queues the swap chain to be reset on the beginning of the next frame. This
  applies any render settings
  */
  virtual void ApplySettings() = 0;

  /**
  Gets the current persistable render settings.
  \param preferences a pointer to a preference set that can be passed into the
  IGame setPreferences method to set the preferences
  */
  virtual void STDMETHODCALLTYPE GetPreferences(IPreferenceSet *
                                                *preferences) = 0;
};

}  // namespace MGDF