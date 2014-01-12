#pragma once

#include <sstream>

namespace MGDF
{

/**
this interface allows subscribers to shut down the host when a fatal error is encountered
*/
class IErrorHandler
{
public:
	/**
	 * This method should be invoked when a fatal error is encountered
	 * \param sender the location or component the error occurred in
	 * \param message details of the fatal error
	 */
	virtual void FatalError( const char *sender, const char *message ) = 0;
};

#define FATALERROR(obj,msg) {\
	std::ostringstream ss;\
	ss << __FILE__ <<  ':' <<__LINE__;\
	std::ostringstream ms;\
	ms << msg;\
	obj->FatalError(ss.str().c_str(),ms.str().c_str());\
}

}