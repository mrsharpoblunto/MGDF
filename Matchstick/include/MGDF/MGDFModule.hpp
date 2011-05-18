#pragma once

#include <MGDF/MGDFDisposable.hpp>

namespace MGDF {

class ISystem;

/**
This class provides an interface for all the game module dll's to use.
To be a valid module, a module must be able to completely save its current state
at any time into a specified file using the savestate method, and completely load
its state from a specified file. The save format is up to the module to implement.
Through the system callback interface, modules are able to stack and unstack 
other modules, aswell as call load, and save events. Modules can also trigger system 
exit events
unless specified these will always be called from the simulation thread
\author gcconner
*/
class IModule
{
public:
	/**
	 This method is required to setup the modulestate and manager callback aswell as do 
	 any initialisation required by the module before it can be used
	 \param workingFolder a folder that the module can read/write data to while its running
	 \return false if the module experiences a fatal error on initialization
	 */
	virtual bool  NewModule(const char *workingFolder)=0;

	/**
	 This method tells the module to load its state from a file
	 \param loadFile the file to load the state from
	 \param saveDataFolder a directory to load any save data additional to that stored in the main save file. This space is shared between all modules
	 running for the current game session so namespacing of files etc. is up to the modules to prevent conflicts
	 \param workingFolder a folder that the module can read/write data to while its running. This space is shared between all modules
	 running for the current game session so namespacing of files etc. is up to the modules to prevent conflicts
	 \return false if the module experiences a fatal error on loading
	*/
	virtual bool  LoadModule(const char *saveDataFolder,const char *workingFolder)=0;

	/**
	 This method is called once per simulation timestep once the game is running and represents the 
	 main loop
	 \param elpasedTime the simulation timestep
 	 \return false if the module experiences a fatal error updating the scene
	*/
	virtual bool  UpdateScene(double elapsedTime)=0;

	/**
	 cleans up the module
  	 \return false if the module experiences a fatal error cleaning up
	*/
	virtual bool Dispose()=0;

	/**
	 renders the current gamestate
	 NOTE: Will be called from the render thread.
	 \param alpha how far between the current and next simulation frame (0-1)
  	 \return false if the module experiences a fatal error drawing the scene
	*/
	virtual bool  DrawScene(double alpha)=0;

	/**
	if the d3d device has been lost this is called, it gives the module an opportunity to
	free any D3DPOOL_DEFAULT resources allocated
	 NOTE: Will be called from the render thread.
	\return false if the module experiences a fatal error handling a lost device
	*/
	virtual bool  DeviceLost()=0;

	/**
	if the device has been lost and then restored, if the module has just been resumed, or if the module has just been initialised. This function is
	for allocating all D3DPOOL_DEFAULT resources
	 NOTE: Will be called from the render thread.
	\return false if the module experiences a fatal error handling a reset device
	*/
	virtual bool  DeviceReset()=0;

	/**
	this method is called immediately before a module is initialised, or  before it
	is resumed and allows the module to set the renderer to a desired state before executing the module
	 NOTE: Will be called from the render thread.
	\return false if the module experiences a fatal error handling a reset device
	*/
	virtual bool  SetDeviceState()=0;

	/**
	this method is called before a module is initialised and checks if the d3d device has the capabilities to
	run the module
	 NOTE: Will be called from the render thread.
	\return true if the device is acceptable, false otherwise
	*/
	virtual bool  CheckDeviceCaps()=0;

	/**
	 This method tells the module to save its entire state into a file, the information
	 in this file should be sufficient to restore the exact same state that it was in
	 when it was saved
	 NOTE Access to the renderer, audio and music managers is prevented as they may be under the control
	 of another module and hence in a state unexpected to the current module
	\param saveDataFolder a directory to save any save data additional to that stored in the main save file
	\return false if the module experiences a fatal error while resuming

	*/
	virtual bool  SaveModule(const char *saveDataFolder)=0;

	/**
	this method is called for all active modules when the a system fatalError event occurs.
	This method gives any modules a chance to clean up any memory/open files etc. as best they can
	before the system crashes
	NOTE: can be called from either the simulation or render threads.
	*/
	virtual void  Panic()=0;

	/**
	allows the system to find out the details of any failed methods, this should be set if any module methods fail.
	*/
	virtual const char * GetLastError()=0;
};

/**
exports the getversion function so the moduleManager can assert if the current MGDF module interface is compatible with it
*/
extern "C" __declspec(dllexport) bool IsCompatibleInterfaceVersion(int);

/**
exports the getmodule function so the modulemanager can get access to instances
of a module
*/
extern "C" __declspec(dllexport) IModule * GetModule(ISystem *);

}


