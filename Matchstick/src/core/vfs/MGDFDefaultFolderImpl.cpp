#include "StdAfx.h"

#include <algorithm>
#include <cctype>//std::tolower
#include "MGDFDefaultFolderImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace vfs {

DefaultFolderImpl::DefaultFolderImpl(const std::wstring &foldername,const std::wstring &physicalPath,VirtualFileSystemComponent *vfs,IFileFilter *filter)
	:FolderBaseImpl(foldername,physicalPath)
{
	_mappedChildren = vfs==nullptr;
	_vfs = vfs;
	_filter = filter;
}

DefaultFolderImpl::~DefaultFolderImpl()
{
}

void DefaultFolderImpl::MapChildren()
{
	if (!_mappedChildren) 
	{
		_vfs->MapDirectory((IFile *)this,_filter);
		_mappedChildren = true;
	}
}

IFileIterator *DefaultFolderImpl::GetIterator()
{
	MapChildren();
	return FolderBaseImpl::GetIterator();
}

IFile *DefaultFolderImpl::GetDescendant(const wchar_t * query)
{
	MapChildren();
	return FolderBaseImpl::GetDescendant(query);
}

IFile *DefaultFolderImpl::GetFirstChild()
{
	MapChildren();
	return FolderBaseImpl::GetFirstChild();
}

IFile *DefaultFolderImpl::GetLastChild()
{
	MapChildren();
	return FolderBaseImpl::GetLastChild();
}

IFile *DefaultFolderImpl::GetChild(const wchar_t *name)
{
	MapChildren();
	return FolderBaseImpl::GetChild(name);
}

unsigned int DefaultFolderImpl::GetChildCount()
{
	MapChildren();
	return FolderBaseImpl::GetChildCount();
}

}}}