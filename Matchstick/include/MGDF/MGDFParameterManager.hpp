#pragma once

namespace MGDF {

/**
 this interface provides a simple means to access and add parameters passed to the application or to modules
 from a range of sources
 \author gcconner
*/
class IParameterManager
{
public:

	/**
	\return whther the parameter manager has a aprameter with the specified name
	*/
	virtual bool  HasParameter(const char *param) const=0;

	/**
	\return get a parameters value, returns NULL if it is not found
	*/
	virtual const char * GetParameter(const char *param) const=0;

	/**
	parse a list of parameters in the floowing format into key value pairs -key:value -key1 -key2:value2 -key3:value3
	*/
	virtual bool  AddParameterString(const char *paramString)=0;
};

}