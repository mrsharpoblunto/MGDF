#pragma once

#include "ZipArchive.hpp"
#include "../../MGDFFolderBaseImpl.hpp"

namespace MGDF { namespace core { namespace vfs { namespace zip {

/**
implementation of a folder in a zipped archive
these files are essentially flyweight objects, all the zip functionality is encapsulated in the ZipArchiveHandler class
*/
class ZipFolderImpl: public FolderBaseImpl
{
public:	
	ZipFolderImpl(const wchar_t *name,IFile *parent,ZipArchive *handler)
		: FolderBaseImpl(name,handler->GetArchiveRoot()->GetPhysicalPath(),parent)
		, _handler( handler )
	{
	}
	virtual ~ZipFolderImpl();

	virtual bool IsArchive() const { return true; }

	virtual const wchar_t *GetArchiveName() const
	{
		return _handler->GetArchiveRoot()->GetName();
	}

	virtual time_t GetLastWriteTime() const
	{
		return _handler->GetArchiveRoot()->GetLastWriteTime();
	}

private:
	ZipArchive *_handler;
};

}}}}