#pragma once

#include "MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFFolderBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

/**
folders map thier children lazily to speed up initial vfs enumeration on program startup, this means that when
a request is made for the children of a folder, it has to call back into the vfs component to enumerate its children at
that time.
*/
class DefaultFolderImpl : public FolderBaseImpl
{
public:
	DefaultFolderImpl(const std::wstring &name,const std::wstring &physicalPath,IFile *parent,VirtualFileSystemComponent *vfs);
	virtual ~DefaultFolderImpl(void);

	virtual IFile *GetChild(const wchar_t *name);
	virtual size_t GetChildCount();
	virtual bool GetAllChildren(const IFileFilter *filter,IFile **childBuffer,size_t *bufferLength);
private:
	bool _mappedChildren;
	VirtualFileSystemComponent *_vfs;
	IFileFilter *_filter;

	void MapChildren();
};

}}}