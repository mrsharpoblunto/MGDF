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
	for ( auto iter = _children->begin(); iter != _children->end(); ++iter ) {
		delete static_cast<FileBaseImpl *>( iter->second );
	}
}

}
}
}
}