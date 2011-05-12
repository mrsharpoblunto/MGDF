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
	virtual int Read(void* buffer,int length);
	virtual void SetPosition(unsigned long pos);
	virtual unsigned long GetPosition() const;
	virtual bool EndOfFile() const;
	virtual unsigned long GetSize();

	virtual const char *GetArchiveName() const;
	virtual const char *GetPhysicalPath() const;
	virtual const char *GetName() const;
private:
	ZipArchive *_handler;
	unsigned int _fileKey;
	bool _isOpen;

	ZipFileImpl(ZipArchive *handler,unsigned int fileKey): _handler(handler), _fileKey(fileKey)
	{
		_isOpen = false;
		_handler->IncRefCount();
	}
};

}}}}