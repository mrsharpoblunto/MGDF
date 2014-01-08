#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{

class FolderBaseImpl : public FileBaseImpl
{
public:
	FolderBaseImpl( const wchar_t *name, const wchar_t *physicalPath, IFile *parent )
		: FileBaseImpl( parent )
		, _name( name )
		, _path( physicalPath ) {
		_ASSERTE( name );
		_ASSERTE( physicalPath );
	}

	FolderBaseImpl( const std::wstring &name, const wchar_t *physicalPath, IFile *parent )
		: FileBaseImpl( parent )
		, _name( name )
		, _path( physicalPath ) {
		_ASSERTE( physicalPath );
	}

	FolderBaseImpl( const std::wstring &name, const std::wstring &physicalPath, IFile *parent )
		: FileBaseImpl( parent )
		, _name( name )
		, _path( physicalPath ) {
	}

	virtual ~FolderBaseImpl() {}

	virtual bool FolderBaseImpl::IsOpen() const {
		return false;
	}
	virtual MGDFError FolderBaseImpl::OpenFile( IFileReader **reader ) {
		return MGDF_ERR_IS_FOLDER;
	}
	
	virtual bool FolderBaseImpl::IsFolder() const {
		return true;
	}
	virtual bool FolderBaseImpl::IsArchive() const {
		return false;
	}
	virtual const wchar_t *FolderBaseImpl::GetArchiveName() const {
		return nullptr;
	}
	virtual const wchar_t *FolderBaseImpl::GetPhysicalPath() const {
		return _path.c_str();
	}
	virtual const wchar_t *GetName() const {
		return _name.c_str();
	}
private:
	std::wstring _name, _path;
};

}
}
}