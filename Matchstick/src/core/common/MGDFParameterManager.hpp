#pragma once

#include <exception>
#include <string>
#include <map>
#include <MGDF/MGDF.hpp>

namespace MGDF
{
namespace core
{

class ParameterManager
{
public:
	static ParameterManager &Instance() {
		static ParameterManager pm;
		return pm;
	}

	/**
	 parse a parameter string into the given string,string hashmap
	 the parameter string must be in the following format
	     -flag [value] -flag ["value"]
	 where [] indicates optional components of the parameter string
	 NOTE: flags and values are case sensitive
	 NOTE: leading and trailing whitespace are stripped from values
	*/
	virtual MGDFError ParseParameters( const std::string &, std::map<std::string, std::string> & );

	virtual bool HasParameter( const char * param ) const;
	virtual const char *GetParameter( const char * param ) const;
	virtual MGDFError AddParameterString( const char *  paramString );

private:
	virtual ~ParameterManager() {}
	std::map<std::string, std::string> _parameters;

};


}
}
