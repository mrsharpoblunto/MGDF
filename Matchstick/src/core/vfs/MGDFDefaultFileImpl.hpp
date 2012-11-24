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
	virtual UINT32 Read(void* buffer,UINT32 length);
	virtual void SetPosition(INT64 pos);
	virtual INT64 GetPosition() const;
	virtual bool EndOfFile() const;
	virtual INT64 GetSize();

	virtual bool IsFolder() const;
	virtual bool IsArchive() const;
	virtual const wchar_t *GetArchiveName() const;
	virtual const wchar_t *GetPhysicalPath() const;
	virtual const wchar_t *GetName() const;

private:
	std::ifstream *_fileStream;
	INT64 _filesize;
	std::wstring _name;
	std::wstring _path;
	ILogger *_logger;
	IErrorHandler *_errorHandler;
};


}}}
