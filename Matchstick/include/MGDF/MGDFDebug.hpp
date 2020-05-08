#pragma once

namespace MGDF {

/**
 * Provide access to the host debug overlay
 */
class IDebug {
 public:
  /**
   * Set some data to display in the overlay
   * \param section The section to display the key:value in
   * \param key The key to display
   * \param value the value to display
   */
  virtual void Set(const char *section, const char *key, const char *value) = 0;
  /**
   * Clear some data out of the overlay
   * \param section to clear the key from
   * \param key The key to clear (if null, clears all keys in the section)
   */
  virtual void Clear(const char *section, const char *key) = 0;

  /**
   * Whether the debug overlay is currently visible
   * \return whether the debug overlay is currently visible
   */
  virtual bool IsShown() const = 0;
  /**
   * Toggle the visibility status of the overlay
   */
  virtual void ToggleShown() = 0;
};

}  // namespace MGDF
