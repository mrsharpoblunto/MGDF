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
	DefaultFolderImpl(std::string foldername,std::string physicalPath="",VirtualFileSystemComponent *vfs=NULL,IFileFilter *filter=NULL);
	virtual ~DefaultFolderImpl(void);

	virtual IFileIterator *GetIterator();
	virtual IFile *GetDescendant(const char * query);
	virtual IFile *GetFirstChild();
	virtual IFile *GetLastChild();
	virtual IFile *GetChild(const char *name);
	virtual unsigned int GetChildCount();
private:
	bool _mappedChildren;
	VirtualFileSystemComponent *_vfs;
	IFileFilter *_filter;

	inline void MapChildren();
};

}}}