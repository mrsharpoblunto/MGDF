#pragma once

#include <vector>
#include <map>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

#include "../common/MGDFSystemComponent.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{

class IVirtualFileSystemComponent: public ISystemComponent, public IVirtualFileSystem
{
public:
	virtual ~IVirtualFileSystemComponent() {}
	virtual bool Mount( const wchar_t * physicalDirectory ) = 0;
	virtual void RegisterArchiveHandler( IArchiveHandler * ) = 0;
};

class DefaultFolderImpl;
struct WCharCmp;

class VirtualFileSystemComponent: public IVirtualFileSystemComponent
{
public:
	VirtualFileSystemComponent();
	virtual ~VirtualFileSystemComponent();

	IFile *GetFile( const wchar_t *logicalPath ) const override;
	IFile *GetRoot() const override;
	bool Mount( const wchar_t * physicalDirectory ) override;
	void RegisterArchiveHandler( IArchiveHandler * ) override;

	void MapChildren( DefaultFolderImpl *parent, std::map<const wchar_t *, IFile *, WCharCmp> &children );
private:
	std::vector<IArchiveHandler *> _archiveHandlers;
	std::multimap<IArchiveHandler *, IFile *> _mappedArchives;

	IFile *_root;
	bool _rootIsArchive;

	IFile *Map( const wchar_t *physicalPath, IFile *parent );
	IArchiveHandler *GetArchiveHandler( const wchar_t *filename );
};

IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl();

}
}
}