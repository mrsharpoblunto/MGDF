#pragma once

#include <MGDF/MGDFVersion.hpp>

namespace MGDF {

/**
Provides information regarding the current game being run
and the preferences associated with that game
*/
class IGame {
 public:
  /**
  get the name of the current game running
  \return the name of the current game running
  */
  virtual const char *GetName() const = 0;

  /**
  get the interface version the current game supports
  \return the interface version the current game supports
  */
  virtual INT32 GetInterfaceVersion() const = 0;

  /**
  get the uid of the current game running
  \return the name of the current game running
  */
  virtual const char *GetUid() const = 0;

  /**
  get the version of the current game running
  \return the version of the current game running
  */
  virtual const Version *GetVersion() const = 0;

  /**
  determine if the game has a preference for the given key
  \param name the preference name
  \return true if the key has an associated value
  */
  virtual bool HasPreference(const char *name) const = 0;

  /**
  determine if the game has a preference for the given key and return its value.
  Preferences can be defined in a games game.json file, or new ones can be
  added/overwritten by calling SetPreference \param name the preference name
  \return the value associated with the key or nullptr if there is no such key
  */
  virtual const char *GetPreference(const char *name) const = 0;

  /**
  change the value of a preference
  \param name the preference name
  \param value the new preference value
  */
  virtual void SetPreference(const char *name, const char *value) = 0;

  /**
  save the current preferences list
  */
  virtual void SavePreferences() const = 0;

  /**
  reload all preferences to thier user independent default values
  */
  virtual void ResetPreferences() = 0;
};

}  // namespace MGDF
