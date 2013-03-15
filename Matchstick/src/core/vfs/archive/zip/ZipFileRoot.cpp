#include "stdafx.h"

#include "ZipFileRoot.hpp"

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