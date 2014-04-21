#pragma once

#include <sstream>

namespace MGDF
{

/**
Provides a means to shut down the host when a fatal error is encountered
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

}