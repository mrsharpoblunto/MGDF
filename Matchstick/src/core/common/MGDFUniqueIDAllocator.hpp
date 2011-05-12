#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

/**
allocates globally unique integer ID numbers
\author gcconner
*/
class MGDF_CORE_COMMON_DLL UniqueIDAllocator
{
public:
	static unsigned int GetID();
private:
	static unsigned int _id;

};

}
}