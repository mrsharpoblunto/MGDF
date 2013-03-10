#pragma once

#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class DefaultFileImpl : public FileBaseImpl
{
public:
	DefaultFileImpl(const std::wstring &name,const std::wstring &physicalPath,IFile *parent,IErrorHandler *handler);
	virtual ~DefaultFileImpl();

	virtual bool IsOpen() const { return (_fileStream!=nullptr); }
	virtual bool OpenFile();
	virtual void CloseFile();
	virtual UINT32 Read(void* buffer,UINT32 length);
	virtual void SetPosition(INT64 pos);
	virtual INT64 GetPosition() const;
	virtual bool EndOfFile() const;
	virtual INT64 GetSize();

	virtual bool IsFolder() const { return false; }
	virtual bool IsArchive() const { return false; }
	virtual const wchar_t *GetArchiveName() const { return nullptr; }
	virtual const wchar_t *GetPhysicalPath() const{ return _path.c_str(); }
	virtual const wchar_t *GetName() const { return _name.c_str(); }
private:
	std::ifstream *_fileStream;
	INT64 _filesize;
	std::wstring _name;
	std::wstring _path;
	IErrorHandler *_errorHandler;
};


}}}
