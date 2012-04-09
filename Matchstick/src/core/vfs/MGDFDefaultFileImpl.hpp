#pragma once

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class DefaultFileImpl : public FileBaseImpl
{
public:
	DefaultFileImpl(const std::wstring &path,ILogger *logger,IErrorHandler *handler);
	virtual ~DefaultFileImpl(void);

	virtual bool IsOpen() const;
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual int Read(void* buffer,int length);
	virtual void SetPosition(unsigned long pos);
	virtual unsigned long GetPosition() const;
	virtual bool EndOfFile() const;
	virtual unsigned long GetSize();

	virtual bool IsFolder() const;
	virtual bool IsArchive() const;
	virtual const wchar_t *GetArchiveName() const;
	virtual const wchar_t *GetPhysicalPath() const;
	virtual const wchar_t *GetName() const;
private:
	std::ifstream *_fileStream;
	unsigned long _filesize;
	std::wstring _name;
	std::wstring _path;
	ILogger *_logger;
	IErrorHandler *_errorHandler;
};


}}}
