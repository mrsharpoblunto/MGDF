#pragma once

#include <string>
#include <exception>

namespace MGDF { namespace core {

/**
error classes
\author gcconner
*/
class MGDFException: public std::exception
{
public:
	MGDFException(std::string message): std::exception(message.c_str()) {};
	virtual ~MGDFException(){}
};

}}