#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs {

class FolderBaseImpl : public FileBaseImpl
{
public:
	FolderBaseImpl(std::string foldername,std::string physicalPath="");
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
	virtual const char *GetArchiveName() const;
	virtual const char *GetPhysicalPath() const;
	virtual const char *GetName() const;

private:
	std::string _name,_path;
};

}}}