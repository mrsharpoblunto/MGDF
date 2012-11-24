#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class FolderBaseImpl : public FileBaseImpl
{
public:
	FolderBaseImpl(const std::wstring &foldername,const std::wstring &physicalPath=L"");
	virtual ~FolderBaseImpl(void);

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
	std::wstring _name,_path;
};

}}}