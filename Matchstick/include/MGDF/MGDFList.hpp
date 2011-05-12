#pragma once

namespace MGDF {

/**
 this class provides a simple readonly wrapper for the std library vector class to enable implementation independant lists to be passed to modules
 \author gcconner
*/
#define DECLARE_LIST(className,typeName) class className## \
{ \
public: \
	virtual unsigned int  Size() const=0; \
	virtual typeName  Get(unsigned int index) const=0; \
};

}
