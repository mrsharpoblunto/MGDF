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
An interface which game modules must implement. The MGDF host will
call the various module methods in order to allow the game module to respond
to events.
Methods prefixed with ST will be invoked by the host from the sim thread, methods 
prefixed with RT will be invoked by the host from the render thread, and methods 
without a prefix may be invoked by the host from either thread.
*/
class IModule
{
public:
	/**
	 Called by the host after a module is first created and should be used
	 to do perform any initialization required by the module
	 \param host the simulation thread host
	 \param workingFolder a folder that the module can read/write data to while its running
	 \return false if the module experiences a fatal error on initialization
	 */
	virtual bool STNew( ISimHost *host, const wchar_t *workingFolder ) = 0;

	/**
	 Called once per simulation timestep by the host once the game is running and represents the
	 main sim loop
	 \param host the simulation thread host
	 \param elapsedTime the simulation timestep
	 \return false if the module experiences a fatal error updating the scene
	*/
	virtual bool STUpdate( ISimHost *host, double elapsedTime ) = 0;

	/**
	 Called by the host to tell the module to cleanup and shutdown as soon as possible. This is invoked when external events
	 such as clicking the windows close button try to close the application. After being invoked it is the modules
	 responsibility to call the ->Shutdown() function as soon as possible in order to actually terminate the
	 application. This function may be called multiple times.
	 \param host the simulation thread host
	 */
	virtual void STShutDown( ISimHost *host ) = 0;

	/**
	 Called by the host to dispose of the module
	 \param host the simulation thread host
	 \return false if the module experiences a fatal error cleaning up
	*/
	virtual bool STDispose( ISimHost *host ) = 0;

	/**
	 Called by the host immediately before the first call to RTDrawScene
	 \param host the render thread host
	 \return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeFirstDraw( IRenderHost *host ) = 0;

	/**
	 Called by the host once per render frame. The module should do any rendering required for the current frame in here
	 \param host the render thread host
	 \param alpha how far between the current and next simulation frame (0-1)
	 \return false if the module experiences a fatal error drawing the scene
	*/
	virtual bool RTDraw( IRenderHost *host, double alpha ) = 0;

	/**
	Called by the host if the swap chain options need to be changed, or the display window needs to be resized. The module should
	clear out all references to the previous back buffer so a new resized backbuffer can be set
	\param host the render thread host
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeBackBufferChange( IRenderHost *host ) = 0;

	/**
	Called by the host after the swap chain options has been changed, or the display window has been resized. The module should
	re-acquire references to the new backbuffer or resize any backbuffer size dependent resources
 	\param host the render thread host
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBackBufferChange( IRenderHost *host ) = 0;

	/**
	Called by the host if the dxgi device needs to be removed. The module should clean out all device dependent resources and
	references to the old d3d device, which will now be invalid.
	\param host the render thread host
	\return false if the module experiences a fatal error
	*/
	virtual bool RTBeforeDeviceReset( IRenderHost *host ) = 0;

	/**
	Called by the host after the dxgi device has been reset, the module should recreate any device dependent resources that
	were removed in RTBeforeDeviceReset
	\param host the render thread host
	\return false if the module experiences a fatal error
	*/
	virtual bool RTDeviceReset( IRenderHost *host ) = 0;

	/**
	Called by the host when a fatalError event occurs.
	This method gives the module a chance to clean up any memory/open files etc. as best it can
	before the host aborts. Methods on the render and sim hosts should not be accessed from within this
	method
	*/
	virtual void  Panic() = 0;
};

/**
Allows a module to assert if the host trying to run the module is compatible with the module. This function is required and will be called by the host before calling any other functions. If this function returns false, the host will abort.
\param Interface the MGDF interface version supported by the host
\return true if the module supports the interface provided by the host
*/
extern "C" __declspec( dllexport ) bool IsCompatibleInterfaceVersion( INT32 Interface );

/**
allows a module to tell the host what D3D11 feature level to try and use when creating the D3D device. This function is required and will be called by the host immediately before the D3D device is created.
\param levels an array supplied to the module to fill with acceptable D3D feature levels
\param levelSize the size of the levels array
\return 0 if the supplied levels array is large enough, otherwise returns the size required
*/
extern "C" __declspec( dllexport ) UINT32 GetCompatibleFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *levelSize );

/**
Factory function which returns an instance of the module to the host. This function is required and will be called if the IsCompatibleInterfaceVersion function returns true
\return an instance of the module interface
*/
extern "C" __declspec( dllexport ) IModule * GetModule();

/**
Allows a module to tell the host if it is going to provide any custom virtual file system handlers. This function is optional and if defined should return a list of all custom handler factories provided by the module
\param list a pointer to an array of characters to store the saves in
\param length the length of the list array
\param logger a callback interface allowing the archive handler to write to the MGDF logs
\param errorHandler a callback interface allowing the archive handler to trigger fatal errors
\return returns true if the supplied list is large enough to contain all the items in the list, otherwise returns false and sets the required size in the length parameter
*/
extern "C" __declspec( dllexport ) bool GetCustomArchiveHandlers( IArchiveHandler **list, UINT32 *length, ILogger *logger, IErrorHandler *errorHandler );


}


