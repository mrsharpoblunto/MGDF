#include "stdafx.h"

#include "ZipFolderImpl.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{
namespace zip
{

ZipFolderImpl::~ZipFolderImpl()
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