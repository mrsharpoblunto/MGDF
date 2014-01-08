#pragma once

#include <d3d11.h>
#include <MGDF/MGDFErrorHandler.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF
{

class ISimHost;
class IRenderHost;

/**
This class provides an interface for game modules to use. The MGDF host will
call the various module methods in order to allow the game module to respond
to events.
Methods prefixed with ST will be invoked by the host from the sim thread, methods 
prefixed with RT will be invoked by the host from the render thread, and methods 
without a prefix may be invoked by the host from either thread.
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
	virtual bool STNew( ISimHost *host, const wchar_t *workingFolder ) = 0;

	/**
	 This method is called once per simulation timestep once the game is running and represents the
	 main loop
	 \param elpasedTime the simulation timestep
	 \return false if the module experiences a fatal error updating the scene
	*/
	virtual bool STUpdate( ISimHost *host, double elapsedTime ) = 0;

	/**
	 This method instructs the module to cleanup and shutdown as soon as possible. This is invoked when external events
	 such as clicking the windows close button try to close the application. After being invoked it is the modules
	 responsibility to call the ->Shutdown() function as soon as possible in order to actually terminate the
	 application. This function may be called multiple times.
	 */
	virtual void STShutDown( ISimHost *host ) = 0;

	/**
	 cleans up the module
	 \return false if the module experiences a fatal error cleaning up
	*/
	virtual bool STDispose( ISimHost *host ) = 0;

	/**
	 runs immediately before the first call to RTDrawScene
	 \return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeFirstDraw( IRenderHost *host ) = 0;

	/**
	 renders the current gamestate
	 \param alpha how far between the current and next simulation frame (0-1)
	 \return false if the module experiences a fatal error drawing the scene
	*/
	virtual bool RTDraw( IRenderHost *host, double alpha ) = 0;

	/**
	If the swap chain options have been changed, or the display window has been resized. The module should
	clear out all references to the previous back buffer so a new resized backbuffer can be set
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeBackBufferChange( IRenderHost *host ) = 0;

	/**
	If the swap chain options have been changed, or the display window has been resized. The module should
	re-acquire references to the new backbuffer or resize any backbuffer size dependent resources
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBackBufferChange( IRenderHost *host ) = 0;

	/**
	If the dxgi device has been removed. The module should clean out all device dependent resources and
	references to the old d3d device, which will now be invalid.
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeDeviceReset( IRenderHost *host ) = 0;

	/**
	After the dxgi device has been reset, the module should recreate any device dependent resources that
	were removed in RTBeforeDeviceReset
	\return false if the module experiences a fatal error
	*/
	virtual bool RTDeviceReset( IRenderHost *host ) = 0;

	/**
	this method is called for the active module when the a  fatalError event occurs.
	This method gives the module a chance to clean up any memory/open files etc. as best it can
	before the  crashes. Methods on the render and sim hosts should not be accessed from within this
	method
	*/
	virtual void  Panic() = 0;
};

/**
exports the IsCompatibleInterfaceVersion function so the module can assert if the current MGDF  interface is compatible with it
\param Interface the MGDF interface version supported by the 
\return true if the module supports the  interface
*/
extern "C" __declspec( dllexport ) bool IsCompatibleInterfaceVersion( INT32 Interface );

/**
allows the  to determine what d3d feature level to try and use when creating the d3d device
\param levels an array supplied to the module to fill with acceptable D3D feature levels
\param levelSize the size of the levels array
\return 0 if the supplied levels array is large enough, otherwise returns the size required.
*/
extern "C" __declspec( dllexport ) UINT32 GetCompatibleFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *levelSize );

/**
exports the getmodule function so the  can get access to instances
of a module
\param  a pointer to the MGDF  interface
\return an instance of the module interface
*/
extern "C" __declspec( dllexport ) IModule * GetModule();

/**
gets a list of all custom handler factories to pass to the vfs.
\param list a pointer to an array of characters to store the saves in.
\param length the length of the list array
\return returns true if the supplied list is large enough to contain all the items in the list, otherwise returns false and sets the required size in the length parameter.
*/
extern "C" __declspec( dllexport ) bool GetCustomArchiveHandlers( IArchiveHandler **list, UINT32 *length, ILogger *logger, IErrorHandler *errorHandler );


}


