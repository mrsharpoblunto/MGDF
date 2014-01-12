#include "stdafx.h"

#include "ZipFileRoot.hpp"

#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace core
{
namespace vfs
{
namespace zip
{

ZipFileRoot::~ZipFileRoot()
{
	if ( !_children ) return;
	for ( auto child : *_children ) {
		delete static_cast<FileBaseImpl *>( child.second );
	}
}

}
}
}
}