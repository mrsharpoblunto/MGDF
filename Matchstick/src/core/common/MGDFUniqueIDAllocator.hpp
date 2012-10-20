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
	static unsigned int GetID();
private:
	static unsigned int _id;

};

}
}