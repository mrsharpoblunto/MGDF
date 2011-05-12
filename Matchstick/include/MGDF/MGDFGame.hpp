#pragma once

#include <MGDF/MGDFVersion.hpp>

namespace MGDF {

/**
this class provides information regarding the current game being run in the engine 
and the preferences associated with that game
\author gcconner
*/
class IGame
{
public:
	/**
	get the name of the current game running
	\return the name of the current game running
	*/
	virtual const char * GetName() const=0;

	/**
	get the interface version the current game supports
	\return the interface version the current game supports
	*/
	virtual int GetInterfaceVersion() const=0;

	/**
	get the uid of the current game running
	\return the name of the current game running
	*/
	virtual const char*  GetUid() const=0;

	/**
	get the version of the current game running
	\return the version of the current game running
	*/
	virtual const Version *  GetVersion() const=0;

	/**
	determine if the game has a preference for the given key
	\name the preference name
	\return true if the key has an associated value
	*/
	virtual bool  HasPreference(const char *name) const=0;

	/**
	determine if the game has a preference for the given key and return its value
	\param name the preference name
	\return the value associated with the key (or "" if there is no such key)
	*/
	virtual const char* GetPreference(const char *name) const=0;

	/**
	change the value of a preference
	\param name the preference name
	\param value the new preference value
	*/
	virtual void  SetPreference(const char *name,const char *value)=0;

	/**
	save the current preferences list
	*/
	virtual void  SavePreferences() const=0;

	/**
	reset all preferences to the settings they had when the engine was first booted up
	(Depending on the design of some modules the effects of this may be
	unpredictable and some of the reset settings may not be applied until the engine is restarted)
	*/
	virtual void  ResetPreferences()=0;
};

}
