#pragma once

namespace MGDF
{

/**
 this class provides a simple readonly wrapper for the std library vector class to enable implementation independant lists to be passed to modules
*/
#define DECLARE_LIST(className,typeName) class className## \
{ \
public: \
	virtual size_t  Size() const=0; \
	virtual typeName  Get(size_t index) const=0; \
};

}
