#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

/**
allocates globally unique integer ID numbers
\author gcconner
*/
class UniqueIDAllocator
{
public:
	static UINT32 GetID();
private:
	static UINT32 _id;

};

}
}