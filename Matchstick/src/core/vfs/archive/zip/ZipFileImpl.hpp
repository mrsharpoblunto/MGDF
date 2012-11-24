#pragma once

#include "ZipArchive.hpp"
#include "../../MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs { namespace zip {

/**
implementation of a file in a zipped archive
these files are essentially flyweight objects, all the zip functionality is encapsulated in the ZipArchiveHandler class
*/
class ZipFileImpl: public FileBaseImpl
{
	friend class ZipArchive;//allow the handler to create instances
public:
	virtual ~ZipFileImpl();
	virtual bool IsFolder() const;
	virtual bool IsArchive() const;

	virtual bool IsOpen() const;
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual UINT32 Read(void* buffer,UINT32 length);
	virtual void SetPosition(INT64 pos);
	virtual INT64 GetPosition() const;
	virtual bool EndOfFile() const;
	virtual INT64 GetSize();
	virtual time_t GetLastWriteTime() const
	{
		return _handler->GetArchiveRoot()->GetLastWriteTime();
	}

	virtual const wchar_t *GetArchiveName() const;
	virtual const wchar_t *GetPhysicalPath() const;
	virtual const wchar_t *GetName() const;
private:
	ZipArchive *_handler;
	UINT32 _fileKey;
	bool _isOpen;

	ZipFileImpl(ZipArchive *handler,UINT32 fileKey): _handler(handler), _fileKey(fileKey)
	{
		_isOpen = false;
		_handler->IncRefCount();
	}
};

}}}}