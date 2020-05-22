#pragma once

#include <MGDF/MGDFGame.hpp>
#include <MGDF/MGDFString.hpp>
#include <MGDF/MGDFVersion.hpp>

namespace MGDF {

/**
A set of preferences to be saved
*/
MIDL_INTERFACE("167C7A2B-AA85-493B-842A-534A14669371")
IPreferenceSet : public IUnknown{};

/**
Provides information regarding the current game being run
and the preferences associated with that game
*/
MIDL_INTERFACE("8BC6BFEF-09BC-4954-AF2F-5D2619F2CEEE")
IGame : public IUnknown {
 public:
  /**
  get the name of the current game running
  \return the name of the current game running
  */
  virtual const char *STDMETHODCALLTYPE GetName() const = 0;

  /**
  get the interface version the current game supports
  \return the interface version the current game supports
  */
  virtual INT32 STDMETHODCALLTYPE GetInterfaceVersion() const = 0;

  /**
  get the uid of the current game running
  \return the name of the current game running
  */
  virtual const char *STDMETHODCALLTYPE GetUid() const = 0;

  /**
  get the version of the current game running
  \return the version of the current game running
  */
  virtual void STDMETHODCALLTYPE GetVersion(Version *) const = 0;

  /**
  determine if the game has a preference for the given key
  \param name the preference name
  \return true if the key has an associated value
  */
  virtual bool STDMETHODCALLTYPE HasPreference(const char *name) const = 0;

  /**
  determine if the game has a preference for the given key and return its value.
  Preferences can be defined in a games game.json file, or new ones can be
  added/overwritten by calling SetPreference
  \param name the preference name
  \param value pointer to the value associated with the key or nullptr if there is no such key
  \return true if the preference key could be found
  */
  virtual HRESULT STDMETHODCALLTYPE GetPreference(const char *name, char *value,
                                                  size_t *size) = 0;

  /**
  change the value of a preference
  \param name the preference name
  \param value the new preference value
  */
  virtual void STDMETHODCALLTYPE SetPreference(const char *name,
                                               const char *value) = 0;

  /**
  Sets a number of preferences in one go. Used for applying system render &
  sound settings
  \param preferences a set of preferences
  */
  virtual void STDMETHODCALLTYPE SetPreferences(IPreferenceSet *
                                                preferences) = 0;

  /**
  save the current preferences list
  */
  virtual void STDMETHODCALLTYPE SavePreferences() const = 0;

  /**
  reload all preferences to their user independent default values
  */
  virtual void STDMETHODCALLTYPE ResetPreferences() = 0;
};

}  // namespace MGDF
