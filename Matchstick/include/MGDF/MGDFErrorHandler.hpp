#pragma once

#include <sstream>

namespace MGDF {

/**
this interface allows subscribers to set the lastError filed on the system and also allows them to shut down the system
*/
class IErrorHandler {
public:
	virtual void SetLastError(const char *sender, UINT32 code,const char *description)=0;
	virtual void FatalError(const char *sender,const char *message)=0;
};

#define FATALERROR(obj,msg) {\
	std::ostringstream ss;\
	ss << __FILE__ <<  ':' <<__LINE__;\
	std::ostringstream ms;\
	ms << msg;\
	obj->FatalError(ss.str().c_str(),ms.str().c_str());\
}

#define SETLASTERROR(obj,code,msg) {\
	std::ostringstream ss;\
	ss << __FILE__ <<  ':' <<__LINE__;\
	std::ostringstream ms;\
	ms << msg;\
	obj->SetLastError(ss.str().c_str(),code,ms.str().c_str());\
}

}