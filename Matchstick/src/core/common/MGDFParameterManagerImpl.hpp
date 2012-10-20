#pragma once

#include <exception>
#include <string>
#include <boost/unordered_map.hpp>
#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

class IParameterManagerImpl
{
public:
	/**
	 parse a parameter string into the given string,string hashmap
	 the parameter string must be in the following format
	     -flag[:value] -flag[:value]
	 where [] indicates optional components of the parameter string
	 NOTE: flags and values are case sensitive 
	 NOTE: leading and trailing whitespace are stripped from values
    */
	virtual void ParseParameters(const std::string &,boost::unordered_map<std::string,std::string> &)=0;
	virtual bool HasParameter(const char * param) const=0;
	virtual const char *GetParameter(const char * param) const=0;
	virtual bool AddParameterString(const char *  paramString)=0;
};

/**
 singleton event log, outputs to a file specified in constants.h
 uses buffered file writes to increase efficiency.
 \author gcconner
*/
class ParameterManager: public IParameterManagerImpl
{
public:
	static ParameterManager *InstancePtr() {
		static ParameterManager pm;
		return &pm;
	}

	virtual void ParseParameters(const std::string &,boost::unordered_map<std::string,std::string> &);

	virtual bool HasParameter(const char * param) const;
	virtual const char *GetParameter(const char * param) const;
	virtual bool AddParameterString(const char *  paramString);

private:
	virtual ~ParameterManager(){}
	boost::unordered_map<std::string,std::string> _parameters;

};

IParameterManagerImpl *GetParameterManagerImpl();

}}
