#pragma once

#include "MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFFolderBaseImpl.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{

/**
folders map thier children lazily to speed up initial vfs enumeration on program startup, this means that when
a request is made for the children of a folder, it has to call back into the vfs component to enumerate its children at
that time.
*/
class DefaultFolderImpl : public FolderBaseImpl
{
public:
	DefaultFolderImpl( const std::wstring &name, const std::wstring &physicalPath, IFile *parent, VirtualFileSystemComponent *vfs );
	virtual ~DefaultFolderImpl( void );

	IFile *GetChild( const wchar_t *name ) const override;
	size_t GetChildCount() const override;
	bool GetAllChildren( const IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const override;
private:
	VirtualFileSystemComponent *_vfs;
	IFileFilter *_filter;

	void MapChildren();
};

}
}
}