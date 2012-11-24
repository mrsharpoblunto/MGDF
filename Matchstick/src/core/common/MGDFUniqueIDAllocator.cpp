#include "StdAfx.h"

#include <limits.h>
#include "MGDFUniqueIDAllocator.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

UINT32 UniqueIDAllocator::_id = 0;

UINT32 UniqueIDAllocator::GetID() {
	if (_id==INT_MAX) {
		_id=0;
	}
	return _id++;
}

}
}
