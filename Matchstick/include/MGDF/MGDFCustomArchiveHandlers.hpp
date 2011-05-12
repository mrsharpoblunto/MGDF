#pragma once

#include <MGDF/MGDFDisposable.hpp>
#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFErrorHandler.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF {

/**
every game can have its own set of custom vfs archive handlers so that its own custom formats can be enumerated by the vfs as normal files.
because the vfs is built before the game boots, this cannot be done when running in the normal module loop, so it has to be done by defining this 
global callback in the games globalcallbacks.dll
\author gcconner
*/
class ICustomArchiveHandlers: public IDisposable
{
public:
	/**
	 gets a list of all custom handler factories to pass to the vfs
	 \param list a pointer to an array of characters to store the saves in.
	 \param length the length of the list array
	 \return returns true if the supplied list is large enough to contain all the items in the list, otherwise returns false and sets the required size in the length parameter.
	*/
	virtual bool GetHandlers(IArchiveHandler **list,unsigned int *length,ILogger *logger,IErrorHandler *errorHandler) const=0;
};

/**
exports the GetCustomArchiveHandlers function so the system can get an instance of a customarchivehandlers class
*/
extern "C" __declspec(dllexport) ICustomArchiveHandlers * GetCustomArchiveHandlers(void);

}