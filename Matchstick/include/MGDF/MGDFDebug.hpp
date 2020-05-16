#pragma once

namespace MGDF {

/**
 * Provide access to the host debug overlay
 */
MIDL_INTERFACE("D69DC4E5-238B-4427-AF03-628818464E41")
IDebug : public IUnknown {
 public:
  /**
   * Set some data to display in the overlay
   * \param section The section to display the key:value in
   * \param key The key to display
   * \param value the value to display
   */
  virtual void STDMETHODCALLTYPE Set(const char *section, const char *key,
                                     const char *value) = 0;
  /**
   * Clear some data out of the overlay
   * \param section to clear the key from
   * \param key The key to clear (if null, clears all keys in the section)
   */
  virtual void STDMETHODCALLTYPE Clear(const char *section,
                                       const char *key) = 0;

  /**
   * Whether the debug overlay is currently visible
   * \return whether the debug overlay is currently visible
   */
  virtual bool STDMETHODCALLTYPE IsShown() const = 0;
  /**
   * Toggle the visibility status of the overlay
   */
  virtual void STDMETHODCALLTYPE ToggleShown() = 0;
};

}  // namespace MGDF
