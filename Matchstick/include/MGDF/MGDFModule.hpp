#pragma once

#include <d3d11.h>
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
	virtual bool  New(const wchar_t *workingFolder)=0;

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
	If the swap chain options have been changed, or the display window has been resized. The module should
	clear out all references to the previous back buffer and resize any backbuffer size dependent resources
	\return false if the module experiences a fatal error
	*/
	virtual bool  BackBufferChanged()=0;

	/**
	 This method instructs the module to cleanup and shutdown as soon as possible. This is invoked when external events
	 such as clicking the windows close button try to close the application. After being invoked it is the modules
	 responsibility to call the system->Shutdown() function as soon as possible in order to actually terminate the 
	 application
	 NOTE: will be called from the render thread.
	 */
	virtual void ShutDown()=0;

	/**
	this method is called for the active module when the a system fatalError event occurs.
	This method gives the module a chance to clean up any memory/open files etc. as best it can
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
exports the IsCompatibleInterfaceVersion function so the module can assert if the current MGDF system interface is compatible with it
\param systemInterface the MGDF interface version supported by the system
\return true if the module supports the system interface
*/
extern "C" __declspec(dllexport) bool IsCompatibleInterfaceVersion(int systemInterface);

/**
allows the system to determine what d3d feature level to try and use when creating the d3d device
\param levels an array supplied to the module to fill with acceptable D3D feature levels
\param levelSize the size of the levels array
\return 0 if the supplied levels array is large enough, otherwise returns the size required.
*/
extern "C" __declspec(dllexport) unsigned int GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,unsigned int *levelSize);

/**
exports the getmodule function so the system can get access to instances
of a module
\param system a pointer to the MGDF system interface
\return an instance of the module interface
*/
extern "C" __declspec(dllexport) IModule * GetModule(ISystem *system);

}


