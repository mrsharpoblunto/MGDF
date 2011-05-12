#pragma once

#include "d3d9.h"

#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFModule.hpp>
#include <MGDF/MGDFGame.hpp>
#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFParameterManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFGameStateMigrator.hpp>
#include <MGDF/MGDFStatisticsManager.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>

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
	 pushes a module onto the top of the modulestack, 
	 \param name the name of the module to push the name of the module is the name of the dll file relative to the modules root directory
	 \param init the module initialiser to pass to the module (can be NULL)
	*/
	virtual void  QueuePushNewModule(const char *name,IModuleInitialiser *init)=0;

	/**
	 pops x modules off the stack, if this is more than the number of modules on the stack 
	 an error is generated. The module that is on the top of the stack after the pop(s)
	 is resumed
	 \param popNum how many modules to pop off the stack
	*/
	virtual void  QueuePopModules(unsigned int popNum)=0;

	/**
	create a new moduleInitialiser object
	\return a new moduleInitialiser object
	*/
	virtual IModuleInitialiser * CreateModuleInitialiser() const=0;

	/**
	 pops the top module off the stack and replaces it with the specified module
	 \param name the name of the module to push the name of the module is the name of the dll file relative to the modules root directory
	 \param init the module initialiser to pass to the module (can be NULL)
	 */
	virtual void  QueueSwapTopModule(const char *name,IModuleInitialiser *init)=0;

	/**
	 tells the manager to save the state of the modulestack and instructs all modules on the
	 stack to save themselves to a file. This also saves the name of the current boot configuration
	 meaning the save file can only be opened again when the modulemanager is running the same
	 configuration (see below). All this information is saved in the save\<some name> folder
	 \param saveName the name of the module save file
	*/
	virtual void  QueueSaveGameState(const char *saveName)=0;

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
	 tells the manager to load the state of the modulestack and instructs all modules on the
	 stack to load themselves from a file. All this information is loaded from the save\<some name>
	 folder
	 This method also checks if the configuration loaded was specified to run with this boot configuration
	 and if possible invokes a save game migrator if the saved boot configuration is an older vesion
	 than the currently executnig version.
	 \param loadName the name of the module load file
	*/
	virtual void  QueueLoadGameState(const char *loadName)=0;

	/**
	get the system logger
	NOTE: Can safely be called from the render thread.
	\return the system logger
	*/
	virtual ILogger * GetLogger() const=0;

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
	get the parameter manager
	\return the parameter manager
	*/
	virtual IParameterManager * GetParameters() const=0;

	/**
	get the audio manager
	\return the audio manager, NULL if the audio subsystem failed to initialize
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
	get the direct3d device object from the system
	NOTE: Can safely be called from the render thread.
	\return the direct3d device object from the system
	*/
	virtual IDirect3DDevice9 * GetD3DDevice() const=0;

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
	tells the modulemanager to shut down the program
	*/
	virtual void  ShutDown(void)=0;

	/**
	allows modules to find out the details of any failed methods
	*/
	virtual const Error * GetLastError() const=0;
};

}