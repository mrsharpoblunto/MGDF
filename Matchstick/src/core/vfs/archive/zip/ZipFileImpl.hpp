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
class ZipFileImpl: public FileBaseImpl, public IFileReader
{
public:
	ZipFileImpl( IFile *parent, ZipArchive *handler, ZipFileHeader && header )
		: FileBaseImpl( parent )
		, _handler( handler )
		, _header( header )
		, _isOpen( false ) {
	}
	virtual ~ZipFileImpl();

	bool IsFolder() const override {
		return false;
	}
	bool IsArchive() const override {
		return true;
	}

	bool IsOpen() const override {
		std::lock_guard<std::mutex> lock( _mutex );
		return _isOpen;
	}

	MGDFError OpenFile( IFileReader **reader ) override;
	void Close() override;
	UINT32 Read( void* buffer, UINT32 length ) override;
	void SetPosition( INT64 pos ) override;
	INT64 GetPosition() const override;
	bool EndOfFile() const override;
	INT64 GetSize() const override {
		return _header.size;
	}

	time_t GetLastWriteTime() const override {
		return _handler->GetArchiveRoot()->GetLastWriteTime();
	}
	const wchar_t *GetArchiveName() const override {
		return _handler->GetArchiveRoot()->GetName();
	}
	const wchar_t *GetPhysicalPath() const override {
		return _handler->GetArchiveRoot()->GetPhysicalPath();
	}
	const wchar_t *GetName() const override {
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