#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFComponents.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

Components::Components()
{
}

Components::~Components()
{
	//warnings output for components that have not been properly cleaned up
	stdext::hash_map<std::string,void *>::iterator iter;
	for (iter=_components.begin();iter!=_components.end();++iter) {
		GetLoggerImpl()->Add(THIS_NAME,"component "+iter->first+" has not been unregistered on shutdown");
	}
}

}}