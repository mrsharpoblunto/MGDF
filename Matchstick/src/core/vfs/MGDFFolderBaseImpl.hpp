#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class FolderBaseImpl : public FileBaseImpl
{
public:
	FolderBaseImpl(std::wstring foldername,std::wstring physicalPath=L"");
	virtual ~FolderBaseImpl(void);

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
	std::wstring _name,_path;
};

}}}