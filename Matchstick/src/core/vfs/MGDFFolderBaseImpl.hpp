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
	}

	FolderBaseImpl( const std::wstring &name, const wchar_t *physicalPath, IFile *parent )
		: FileBaseImpl( parent )
		, _name( name )
		, _path( physicalPath ) {
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
	virtual bool FolderBaseImpl::OpenFile() {
		return false;
	}
	virtual void FolderBaseImpl::CloseFile() {}
	virtual UINT32 FolderBaseImpl::Read( void* buffer, UINT32 length ) {
		return 0U;
	}
	virtual void FolderBaseImpl::SetPosition( INT64 pos ) {}
	virtual INT64 FolderBaseImpl::GetPosition() const {
		return 0;
	}
	virtual bool FolderBaseImpl::EndOfFile() const {
		return true;
	}
	virtual INT64 FolderBaseImpl::GetSize() {
		return 0;
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