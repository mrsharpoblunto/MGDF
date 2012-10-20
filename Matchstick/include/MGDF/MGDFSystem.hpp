#pragma once

#include "d3d11.h"

#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFModule.hpp>
#include <MGDF/MGDFGame.hpp>
#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFParameterManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFStatisticsManager.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>
#include <MGDF/MGDFTimer.hpp>

namespace MGDF {

DECLARE_LIST(IStringList,const char *)

/**
 This class provides a callback interface for the modules to interact with the system
 moduleManager. The modulemanager operates like a stack where the the module on top of
 the stack is currently being executed, with all stacked modules below being suspended
 all methods that change the state of the modules and can be called by modules have to
 be queued to resolve circular depandancy issues.
 Unless specified these methods are not thread safe and cannot be called from the render thread.
 \author gcconner
*/
class ISystem
{
public:

	/**
	 tells the system to provide a location on disk to save the current game data. After saving the data
	 it is required that CompleteSave is called using the same saveName parameter.
	 \param saveName the name of the module save file. Only alphanumeric characters and space are valid characters.
	 \param saveBuffer the buffer to fill in the supplied save directory
	 \param size the size of saveBuffer, if saveBuffer is too small, size will be changed to the size required.
	 \return 0 if saveBuffer is large enough to fit the supplied save directory, otherwise returns the size required. If the saveName
		is invalid, the function returns -1
	*/
	virtual int BeginSave(const char *saveName, wchar_t *saveBuffer, unsigned int *size)=0;

	/**
	 finalizes the save data for a matching call to BeginSave
	 \return true if the saveName was in a pending state and was completed successfully, if there was a problem, or the saveName
	 didn't exist then false is returned.
	 */
	virtual bool CompleteSave(const char *saveName)=0;
	
	/**
	 populates the supplied vector with the names of all saved instances of this configuration
	 The names returned in this list represent all the valid arguments to queueLoadState for the 
	 current configuration
	 \param list the list to fill with save names
	*/
	virtual const IStringList * GetSaves() const=0;

	/**
	 deletes a selected save game from the hard drive
	 */
	virtual void RemoveSave(const char *saveName)=0;

	/**
	 tells the system to find the location on disk for the specified save game
	 \param saveName the name of the module save file
	 \param loadBuffer the buffer to fill in the supplied save directory
	 \param size the size of saveBuffer, if saveBuffer is too small, size will be changed to the size required.
	 \param version the version number of the save game. can be useful for migrating save games.
	 \return 0 if saveBuffer is large enough to fit the supplied load directory, otherwise returns the size required. If the saveName
		is invalid, the function returns -1
	*/
	virtual int Load(const char *saveName, wchar_t *loadBuffer, unsigned int *size,Version &version)=0;

	/**
	get the system logger
	NOTE: Can safely be called from the render thread.
	\return the system logger
	*/
	virtual ILogger * GetLogger() const=0;

	/**
	get the system timer
	NOTE: Can safely be called from the render thread.
	\return the system timer
	*/
	virtual ITimer * GetTimer() const=0;

	/**
	get the virtual file system
	\return the virtual file system
	*/
	virtual IVirtualFileSystem * GetVFS() const=0;

	/**
	get information regarding the current game and its preferences
	\return object containing information regarding the game and its preferences
	*/
	virtual IGame * GetGame() const=0;

	/**
	get the audio manager
	\return the audio manager, nullptr if the audio subsystem failed to initialize
	*/
	virtual ISoundManager * GetSound() const=0;

	/**
	get the statistics manager
	\return the statistics manager
	*/
	virtual IStatisticsManager * GetStatistics() const=0;

	/**
	get the input manager
	\return the input manager
	*/
	virtual IInputManager * GetInput() const=0;

	/**
	get the graphics manager
	NOTE: Can safely be called from the render thread.
	\return the graphics manager
	*/
	virtual IGraphicsManager * GetGraphics() const=0;

	/**
	gets the current version of the framework
	*/
	virtual const Version * GetMGDFVersion()  const=0;

	/**
	inform the system a fatal error has occured (how this is handled is up to the 
	system) but would propably result in an output then closure of the program.
	\param message a description of the error encountered
	*/
	virtual void  FatalError(const char *sender,const char *message)=0;

	/**
	tells the system to shut down the game immediately
	*/
	virtual void ShutDown(void)=0;

	/**
	tells the system to invoke the modules shutdown callback
	*/
	virtual void QueueShutDown(void)=0;

	/**
	allows modules to find out the details of any failed methods
	*/
	virtual const Error * GetLastError() const=0;
};

}