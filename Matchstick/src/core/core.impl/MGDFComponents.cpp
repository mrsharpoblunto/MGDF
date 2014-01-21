#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFComponents.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

Components::Components()
{
}

Components::~Components()
{
	//warnings output for components that have not been properly cleaned up
	for ( auto &component : _components ) {
		LOG( "component " << component.first.name() << " has not been unregistered on shutdown", LOG_ERROR );
	}
}

}
}