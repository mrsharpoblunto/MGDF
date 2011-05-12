#pragma once

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class DefaultFileImpl : public FileBaseImpl
{
public:
	DefaultFileImpl(std::string path,ILogger *logger,IErrorHandler *handler);
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
	virtual const char *GetArchiveName() const;
	virtual const char *GetPhysicalPath() const;
	virtual const char *GetName() const;
private:
	std::ifstream *_fileStream;
	unsigned long _filesize;
	std::string _name;
	std::string _path;
	ILogger *_logger;
	IErrorHandler *_errorHandler;
};


}}}
