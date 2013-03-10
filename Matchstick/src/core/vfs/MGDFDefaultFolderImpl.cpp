#include "StdAfx.h"

#include <algorithm>
#include "MGDFDefaultFolderImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

DefaultFolderImpl::DefaultFolderImpl(const std::wstring &name,const std::wstring &physicalPath,IFile *parent,VirtualFileSystemComponent *vfs)
	: FolderBaseImpl(name,physicalPath,parent)
	, _mappedChildren( false )
	, _vfs( vfs )
{
}

DefaultFolderImpl::~DefaultFolderImpl()
{
	if  (!_children) return;

	for (auto iter=_children->begin();iter!=_children->end();++iter) {
		// don't delete archives as we will explicitly pass them off to
		// the archive handler that created them in order to clean them up
		if (!iter->second->IsArchive()) {
			delete static_cast<FileBaseImpl *>(iter->second);
		}
	}
}

void DefaultFolderImpl::MapChildren()
{
	if (!_mappedChildren) {
		_vfs->MapChildren(this);
		_mappedChildren = true;
	}
}

IFile *DefaultFolderImpl::GetChild(const wchar_t *name)
{
	MapChildren();
	return FolderBaseImpl::GetChild(name);
}

size_t DefaultFolderImpl::GetChildCount()
{
	MapChildren();
	return FolderBaseImpl::GetChildCount();
}

bool DefaultFolderImpl::GetAllChildren(const IFileFilter *filter,IFile **childBuffer,size_t *bufferLength)
{
	MapChildren();
	return FolderBaseImpl::GetAllChildren(filter,childBuffer,bufferLength);
}

}}}