#pragma once

#include "ZipArchive.hpp"
#include "../../MGDFFileBaseImpl.hpp"

namespace MGDF
{
namespace core
{
namespace vfs
{
namespace zip
{

/**
implementation of a file in a zipped archive
*/
class ZipFileImpl: public FileBaseImpl
{
public:
	ZipFileImpl( IFile *parent, ZipArchive *handler, ZipFileHeader && header )
		: FileBaseImpl( parent )
		, _handler( handler )
		, _header( header )
		, _isOpen( false ) {
	}
	virtual ~ZipFileImpl();

	virtual bool IsFolder() const {
		return false;
	}
	virtual bool IsArchive() const {
		return true;
	}

	virtual bool IsOpen() const {
		return _isOpen;
	}
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual UINT32 Read( void* buffer, UINT32 length );
	virtual void SetPosition( INT64 pos );
	virtual INT64 GetPosition() const;
	virtual bool EndOfFile() const;
	virtual INT64 GetSize() {
		return _header.size;
	}

	virtual time_t GetLastWriteTime() const {
		return _handler->GetArchiveRoot()->GetLastWriteTime();
	}
	virtual const wchar_t *GetArchiveName() const {
		return _handler->GetArchiveRoot()->GetName();
	}
	virtual const wchar_t *GetPhysicalPath() const {
		return _handler->GetArchiveRoot()->GetPhysicalPath();
	}
	virtual const wchar_t *GetName() const {
		return _header.name.c_str();
	}
private:
	ZipArchive *_handler;
	ZipFileHeader _header;
	ZipFileData _data;
	bool _isOpen;
};

}
}
}
}