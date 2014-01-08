#include "StdAfx.h"

#include <algorithm>
#include "MGDFDefaultFolderImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{
namespace vfs
{

DefaultFolderImpl::DefaultFolderImpl( const std::wstring &name, const std::wstring &physicalPath, IFile *parent, VirtualFileSystemComponent *vfs )
	: FolderBaseImpl( name, physicalPath, parent )
	, _vfs( vfs )
{
	_ASSERTE( vfs );
}

DefaultFolderImpl::~DefaultFolderImpl()
{
	if ( !_children ) return;

	for ( auto child : *_children ) {
		// don't delete archives as we will explicitly pass them off to
		// the archive handler that created them in order to clean them up
		if ( !child.second->IsArchive() ) {
			delete static_cast<FileBaseImpl *>( child.second );
		}
	}
}

void DefaultFolderImpl::MapChildren()
{
	boost::mutex::scoped_lock lock( _mutex );
	if ( !_children ) {
		auto children = new std::map<const wchar_t *, IFile *, WCharCmp>();
		_vfs->MapChildren( this, *children );
		_children = children;
	}
}

IFile *DefaultFolderImpl::GetChild( const wchar_t *name ) const
{
	if ( !name ) return nullptr;

	const_cast<DefaultFolderImpl *>(this)->MapChildren();
	return FolderBaseImpl::GetChild( name );
}

size_t DefaultFolderImpl::GetChildCount()  const
{
	const_cast<DefaultFolderImpl *>(this)->MapChildren();
	return FolderBaseImpl::GetChildCount();
}

bool DefaultFolderImpl::GetAllChildren( const IFileFilter *filter, IFile **childBuffer, size_t *bufferLength )  const
{
	const_cast<DefaultFolderImpl *>(this)->MapChildren();
	return FolderBaseImpl::GetAllChildren( filter, childBuffer, bufferLength );
}

}
}
}