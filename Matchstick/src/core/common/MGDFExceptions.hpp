#pragma once

#include <MGDF/MGDFError.hpp>
#include <string>
#include <exception>

namespace MGDF
{
namespace core
{

/**
error classes
\author gcconner
*/
class MGDFException: public std::exception
{
public:
	MGDFException( MGDFError code, const std::string &message ) 
		: std::exception( message.c_str() )
		, _code( code )
	{}
	virtual ~MGDFException() {}

	MGDFError Code() const { return _code ; }
private:
	MGDFError _code;
};

}
}