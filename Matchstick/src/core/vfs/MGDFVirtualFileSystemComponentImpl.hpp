#pragma once

#include <filesystem>
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

	IFile *GetFile( const wchar_t *logicalPath ) const override final;
	IFile *GetRoot() const override final;
	bool Mount( const wchar_t * physicalDirectory ) override final;
	void RegisterArchiveHandler( IArchiveHandler * ) override final;

	void MapChildren( DefaultFolderImpl *parent, std::map<const wchar_t *, IFile *, WCharCmp> &children );
private:
	std::vector<IArchiveHandler *> _archiveHandlers;
	std::multimap<IArchiveHandler *, IFile *> _mappedArchives;

	IFile *_root;
	bool _rootIsArchive;

	IFile *Map( const std::tr2::sys::wpath &path, IFile *parent );
	IArchiveHandler *GetArchiveHandler( const std::wstring &path );
};

IVirtualFileSystemComponent *CreateVirtualFileSystemComponentImpl();

}
}
}