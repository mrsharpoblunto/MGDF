#include "StdAfx.h"
#include "MGDFModuleInitialiserImpl.hpp"
#include "../common/MGDFParameterManagerImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

ModuleInitialiser::ModuleInitialiser(void)
{
}

ModuleInitialiser::~ModuleInitialiser(void)
{
}

void ModuleInitialiser::Dispose()
{
	delete this;
}

void ModuleInitialiser::AddParam(const char *key,const char *value)
{
	std::string k(key);
	_params[k] = value;
}

void ModuleInitialiser::RemoveParam(const char *key)
{
	std::string k(key);
	stdext::hash_map<std::string,std::string>::iterator iter = _params.find(k);

	if (iter!=_params.end()) {
		_params.erase(iter);
	}
}

const char *ModuleInitialiser::GetParam(const char *key) const
{
	std::string k(key);
	stdext::hash_map<std::string,std::string>::const_iterator iter = _params.find(k);
	if (iter!=_params.end()) {
		return iter->second.c_str();
	}
	else {
		return NULL;
	}
}

void ModuleInitialiser::AddParams(std::string params)
{
	GetParameterManagerImpl()->ParseParameters(params,_params);
}

}}
